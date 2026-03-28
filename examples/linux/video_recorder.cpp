#include <ccap.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>

#if defined(__linux__) || defined(__unix__) || defined(_POSIX_VERSION)
#include <getopt.h>
#include <unistd.h>
#include <sys/time.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#endif

struct VideoEncoder {
    AVFormatContext* fmt_ctx = nullptr;
    AVCodecContext* codec_ctx = nullptr;
    AVStream* video_stream = nullptr;
    AVPacket* pkt = nullptr;
    AVFrame* frame = nullptr;
    struct SwsContext* sws_ctx = nullptr;
    
    int width = 0;
    int height = 0;
    int fps = 30;
    int64_t frame_count = 0;
    bool initialized = false;
};

bool init_encoder(VideoEncoder* enc, const char* filename, int width, int height, int fps) {
    enc->width = width;
    enc->height = height;
    enc->fps = fps;
    
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        std::cerr << "H.264 encoder not found!" << std::endl;
        return false;
    }
    
    enc->fmt_ctx = avformat_alloc_context();
    if (!enc->fmt_ctx) {
        std::cerr << "Could not allocate format context" << std::endl;
        return false;
    }
    
    AVOutputFormat* output_format = av_guess_format(nullptr, filename, nullptr);
    if (!output_format) {
        std::cerr << "Could not guess output format" << std::endl;
        return false;
    }
    
    enc->fmt_ctx->oformat = output_format;
    snprintf(enc->fmt_ctx->filename, sizeof(enc->fmt_ctx->filename), "%s", filename);
    
    enc->video_stream = avformat_new_stream(enc->fmt_ctx, codec);
    if (!enc->video_stream) {
        std::cerr << "Could not create new stream" << std::endl;
        return false;
    }
    
    enc->codec_ctx = avcodec_alloc_context3(codec);
    if (!enc->codec_ctx) {
        std::cerr << "Could not allocate codec context" << std::endl;
        return false;
    }
    
    enc->codec_ctx->width = width;
    enc->codec_ctx->height = height;
    enc->codec_ctx->time_base = {1, fps};
    enc->codec_ctx->framerate = {fps, 1};
    enc->codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    enc->codec_ctx->bit_rate = 2500000;
    enc->codec_ctx->gop_size = 12;
    
    av_opt_set(enc->codec_ctx->priv_data, "preset", "medium", 0);
    av_opt_set(enc->codec_ctx->priv_data, "crf", "23", 0);
    
    if (enc->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        enc->codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    
    if (avcodec_open2(enc->codec_ctx, codec, nullptr) < 0) {
        std::cerr << "Could not open codec" << std::endl;
        return false;
    }
    
    if (avcodec_parameters_from_context(enc->video_stream->codecpar, enc->codec_ctx) < 0) {
        std::cerr << "Could not copy codec parameters" << std::endl;
        return false;
    }
    
    enc->video_stream->time_base = enc->codec_ctx->time_base;
    
    if (avio_open(&enc->fmt_ctx->pb, filename, AVIO_FLAG_WRITE) < 0) {
        std::cerr << "Could not open output file: " << filename << std::endl;
        return false;
    }
    
    if (avformat_write_header(enc->fmt_ctx, nullptr) < 0) {
        std::cerr << "Error writing header" << std::endl;
        return false;
    }
    
    enc->pkt = av_packet_alloc();
    enc->frame = av_frame_alloc();
    
    if (!enc->pkt || !enc->frame) {
        std::cerr << "Could not allocate packet or frame" << std::endl;
        return false;
    }
    
    enc->frame->format = enc->codec_ctx->pix_fmt;
    enc->frame->width = width;
    enc->frame->height = height;
    
    if (av_frame_get_buffer(enc->frame, 32) < 0) {
        std::cerr << "Could not allocate frame buffer" << std::endl;
        return false;
    }
    
    enc->initialized = true;
    std::cout << "Video encoder initialized: " << width << "x" << height << " @" << fps << " fps" << std::endl;
    return true;
}

void encode_frame(VideoEncoder* enc, const uint8_t* rgb_data, int width, int height, int stride) {
    if (!enc->initialized) {
        std::cerr << "Encoder not initialized" << std::endl;
        return;
    }
    
    if (av_frame_make_writable(enc->frame) < 0) {
        std::cerr << "Frame not writable" << std::endl;
        return;
    }
    
    if (!enc->sws_ctx) {
        enc->sws_ctx = sws_getContext(width, height, AV_PIX_FMT_RGB24,
                                       enc->codec_ctx->width, enc->codec_ctx->height,
                                       enc->codec_ctx->pix_fmt, SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!enc->sws_ctx) {
            std::cerr << "Could not initialize conversion context" << std::endl;
            return;
        }
    }
    
    uint8_t* src_data[] = {const_cast<uint8_t*>(rgb_data)};
    int src_linesize[] = {stride};
    
    sws_scale(enc->sws_ctx, src_data, src_linesize, 0, height,
              enc->frame->data, enc->frame->linesize);
    
    enc->frame->pts = enc->frame_count;
    enc->frame_count++;
    
    if (avcodec_send_frame(enc->codec_ctx, enc->frame) < 0) {
        std::cerr << "Error sending frame" << std::endl;
        return;
    }
    
    while (avcodec_receive_packet(enc->codec_ctx, enc->pkt) >= 0) {
        enc->pkt->stream_index = enc->video_stream->index;
        av_interleaved_write_frame(enc->fmt_ctx, enc->pkt);
        av_packet_unref(enc->pkt);
    }
}

void flush_encoder(VideoEncoder* enc) {
    if (!enc->initialized) return;
    
    avcodec_send_frame(enc->codec_ctx, nullptr);
    
    while (avcodec_receive_packet(enc->codec_ctx, enc->pkt) >= 0) {
        enc->pkt->stream_index = enc->video_stream->index;
        av_interleaved_write_frame(enc->fmt_ctx, enc->pkt);
        av_packet_unref(enc->pkt);
    }
    
    av_write_trailer(enc->fmt_ctx);
}

void close_encoder(VideoEncoder* enc) {
    if (!enc->initialized) return;
    
    if (enc->sws_ctx) {
        sws_freeContext(enc->sws_ctx);
        enc->sws_ctx = nullptr;
    }
    
    if (enc->fmt_ctx && enc->fmt_ctx->pb) {
        avio_closep(&enc->fmt_ctx->pb);
    }
    
    if (enc->codec_ctx) {
        avcodec_free_context(&enc->codec_ctx);
    }
    
    if (enc->fmt_ctx) {
        avformat_free_context(enc->fmt_ctx);
    }
    
    if (enc->pkt) {
        av_packet_free(&enc->pkt);
    }
    
    if (enc->frame) {
        av_frame_free(&enc->frame);
    }
    
    enc->initialized = false;
    std::cout << "Video encoder closed" << std::endl;
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help              Show this help message" << std::endl;
    std::cout << "  -l, --list              List available cameras" << std::endl;
    std::cout << "  -d, --device <index>    Select camera by index (default: 0)" << std::endl;
    std::cout << "  -o, --output <file>     Output video file (default: output.mp4)" << std::endl;
    std::cout << "  -w, --width <pixels>   Video width (default: 640)" << std::endl;
    std::cout << "  -H, --height <pixels>   Video height (default: 480)" << std::endl;
    std::cout << "  -f, --fps <rate>        Frame rate (default: 30)" << std::endl;
    std::cout << "  -t, --duration <sec>   Recording duration in seconds (default: 10)" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  " << program_name << " -o video.mp4 -w 1280 -H 720 -f 30 -t 60" << std::endl;
}

int main(int argc, char** argv) {
#if !defined(__linux__) && !defined(__unix__) && !defined(_POSIX_VERSION)
    std::cerr << "This example is designed for Linux only" << std::endl;
    return 1;
#endif
    
    int camera_index = 0;
    std::string output_file = "output.mp4";
    int width = 640;
    int height = 480;
    int fps = 30;
    int duration = 10;
    bool list_cameras = false;
    
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"list", no_argument, 0, 'l'},
        {"device", required_argument, 0, 'd'},
        {"output", required_argument, 0, 'o'},
        {"width", required_argument, 0, 'w'},
        {"height", required_argument, 0, 'H'},
        {"fps", required_argument, 0, 'f'},
        {"duration", required_argument, 0, 't'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "hld:o:w:H:f:t:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 'l':
                list_cameras = true;
                break;
            case 'd':
                camera_index = std::stoi(optarg);
                break;
            case 'o':
                output_file = optarg;
                break;
            case 'w':
                width = std::stoi(optarg);
                break;
            case 'H':
                height = std::stoi(optarg);
                break;
            case 'f':
                fps = std::stoi(optarg);
                break;
            case 't':
                duration = std::stoi(optarg);
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    ccap::setErrorCallback([](ccap::ErrorCode errorCode, std::string_view description) {
        std::cerr << "Camera Error - Code: " << static_cast<int>(errorCode)
                  << ", Description: " << description << std::endl;
    });
    
    ccap::Provider camera_provider;
    
    if (list_cameras) {
        std::cout << "Available cameras:" << std::endl;
        auto devices = camera_provider.findDeviceNames();
        for (size_t i = 0; i < devices.size(); ++i) {
            std::cout << "  [" << i << "] " << devices[i] << std::endl;
        }
        return 0;
    }
    
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Camera index: " << camera_index << std::endl;
    std::cout << "  Output file: " << output_file << std::endl;
    std::cout << "  Resolution: " << width << "x" << height << std::endl;
    std::cout << "  Frame rate: " << fps << " fps" << std::endl;
    std::cout << "  Duration: " << duration << " seconds" << std::endl;
    
    camera_provider.set(ccap::PropertyName::Width, width);
    camera_provider.set(ccap::PropertyName::Height, height);
    camera_provider.set(ccap::PropertyName::FrameRate, fps);
    camera_provider.set(ccap::PropertyName::PixelFormatOutput, ccap::PixelFormat::RGB24);
    
    if (!camera_provider.open(camera_index, true)) {
        std::cerr << "Failed to open camera " << camera_index << std::endl;
        return 1;
    }
    
    if (!camera_provider.isStarted()) {
        std::cerr << "Failed to start camera!" << std::endl;
        return 1;
    }
    
    int real_width = static_cast<int>(camera_provider.get(ccap::PropertyName::Width));
    int real_height = static_cast<int>(camera_provider.get(ccap::PropertyName::Height));
    double real_fps = camera_provider.get(ccap::PropertyName::FrameRate);
    
    std::cout << "Camera started successfully" << std::endl;
    std::cout << "Actual resolution: " << real_width << "x" << real_height << std::endl;
    std::cout << "Actual frame rate: " << real_fps << " fps" << std::endl;
    
    VideoEncoder encoder;
    if (!init_encoder(&encoder, output_file.c_str(), real_width, real_height, static_cast<int>(real_fps))) {
        std::cerr << "Failed to initialize video encoder" << std::endl;
        camera_provider.stop();
        camera_provider.close();
        return 1;
    }
    
    std::cout << "Recording started..." << std::endl;
    std::cout << "Press Ctrl+C to stop early" << std::endl;
    
    time_t start_time = time(nullptr);
    int frame_count = 0;
    
    while (true) {
        auto frame = camera_provider.grab(3000);
        if (!frame) {
            std::cerr << "Failed to grab frame" << std::endl;
            continue;
        }
        
        if (frame->pixelFormat != ccap::PixelFormat::RGB24) {
            std::cerr << "Expected RGB24 format, got: " 
                      << ccap::pixelFormatToString(frame->pixelFormat).data() << std::endl;
            continue;
        }
        
        encode_frame(&encoder, frame->data[0], frame->width, frame->height, frame->stride[0]);
        frame_count++;
        
        time_t elapsed = time(nullptr) - start_time;
        
        if (elapsed >= duration) {
            std::cout << "Recording duration reached" << std::endl;
            break;
        }
        
        if (frame_count % static_cast<int>(real_fps) == 0) {
            std::cout << "\rRecorded: " << elapsed << "s / " << duration << "s, "
                      << frame_count << " frames" << std::flush;
        }
    }
    
    std::cout << std::endl;
    std::cout << "Total frames captured: " << frame_count << std::endl;
    
    flush_encoder(&encoder);
    close_encoder(&encoder);
    
    camera_provider.stop();
    camera_provider.close();
    
    std::cout << "Video saved to: " << output_file << std::endl;
    
    return 0;
}
