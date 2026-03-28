# Linux 下摄像头视频录制完整指南

## 项目简介

本项目基于 [ccap (CameraCapture)](https://github.com/wysaid/CameraCapture) 库，这是一个高性能、轻量级的跨平台相机捕获库，支持 Windows、macOS 和 Linux 平台。在 Linux 平台上，ccap 使用 V4L2（Video4Linux2）作为底层捕获接口。

本 Fork 仓库包含了专门为 Linux 设计的视频录制示例，可以帮助你快速实现摄像头视频捕获并保存为 H.264 编码的 MP4 文件。

## 功能特性

- ✅ 跨平台支持：Windows、macOS、Linux
- ✅ 使用 V4L2 进行摄像头捕获
- ✅ 支持多种分辨率和帧率
- ✅ H.264 视频编码（使用 FFmpeg/libavcodec）
- ✅ RGB24 像素格式转换
- ✅ 实时录制并显示进度
- ✅ 完整的 CMake 构建系统

## 系统要求

### 硬件要求
- 支持 V4L2 的 USB 摄像头或内置摄像头
- Linux 内核 2.6.26 及以上版本

### 软件要求
- GCC 7+ 或 Clang 6+
- CMake 3.14+
- FFmpeg 开发库

### FFmpeg 依赖安装

**Ubuntu / Debian:**
```bash
sudo apt-get update
sudo apt-get install build-essential cmake
sudo apt-get install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev
```

**Fedora / RHEL / CentOS:**
```bash
sudo dnf install gcc gcc-c++ cmake
sudo dnf install ffmpeg-devel
```

**Arch Linux:**
```bash
sudo pacman -S base-devel cmake ffmpeg
```

**openSUSE:**
```bash
sudo zypper install gcc gcc-c++ cmake
sudo zypper install ffmpeg-devel
```

## 快速开始

### 1. 克隆仓库

```bash
git clone https://github.com/ChrisLYF/CameraCapture.git
cd CameraCapture
```

### 2. 创建构建目录

```bash
mkdir build && cd build
```

### 3. 配置 CMake

```bash
cmake .. -DCCAP_BUILD_EXAMPLES=ON
```

### 4. 编译项目

```bash
cmake --build . -j$(nproc)
```

### 5. 运行示例

#### 列出可用摄像头

```bash
./linux_camera_list
```

输出示例：
```
Searching for available cameras...
Found 1 camera(s):

Camera 0:
  Name: /dev/video0
  Supported resolutions:
    640x480
    1280x720
    1920x1080
  Supported pixel formats:
    RGB24
    YUV420
```

#### 录制视频

```bash
./video_recorder -o output.mp4
```

录制 10 秒的默认视频（640x480@30fps）。

#### 自定义参数录制

```bash
./video_recorder -o my_video.mp4 -d 1 -H 720 -f 30 -t 60
```

参数说明：
- `-o, --output`: 输出文件名（默认：output.mp4）
- `-d, --device`: 摄像头索引（默认：0）
- `-w, --width`: 视频宽度（默认：640）
- `-H, --height`: 视频高度（默认：480）
- `-f, --fps`: 帧率（默认：30）
- `-t, --duration`: 录制时长/秒（默认：10）

## 代码示例

### 基本用法（C++）

```cpp
#include <ccap.h>
#include <iostream>

int main() {
    ccap::Provider camera_provider;
    
    // 列出所有可用摄像头
    auto devices = camera_provider.findDeviceNames();
    for (size_t i = 0; i < devices.size(); ++i) {
        std::cout << "[" << i << "] " << devices[i] << std::endl;
    }
    
    // 打开第一个摄像头
    if (camera_provider.open(0, true)) {
        // 设置捕获参数
        camera_provider.set(ccap::PropertyName::Width, 1280);
        camera_provider.set(ccap::PropertyName::Height, 720);
        camera_provider.set(ccap::PropertyName::FrameRate, 30);
        
        // 捕获帧
        auto frame = camera_provider.grab(3000);
        if (frame) {
            std::cout << "捕获帧: " << frame->width << "x" << frame->height << std::endl;
        }
    }
    
    return 0;
}
```

### 集成到你的 CMake 项目

```cmake
cmake_minimum_required(VERSION 3.14)
project(my_video_project)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
    ccap
    GIT_REPOSITORY https://github.com/ChrisLYF/CameraCapture.git
    GIT_TAG main
)
FetchContent_MakeAvailable(ccap)

add_executable(my_video_app main.cpp)

target_link_libraries(my_video_app PRIVATE ccap::ccap)
```

## 视频编码详情

本示例使用 FFmpeg 的 libavcodec 库进行 H.264 视频编码：

- **编码器**: libx264 (H.264)
- **编码预设**: medium（平衡速度和质量的预设）
- **CRF 值**: 23（中等质量）
- **容器格式**: MP4
- **像素格式**: YUV420P

### 编码参数调整

如果你需要调整视频质量或文件大小，可以在 `video_recorder.cpp` 中修改编码参数：

```cpp
// 更高质量（更低 CRF 值）
enc->codec_ctx->bit_rate = 5000000;  // 5 Mbps
av_opt_set(enc->codec_ctx->priv_data, "crf", "18", 0);  // CRF 18

// 更快编码
av_opt_set(enc->codec_ctx->priv_data, "preset", "fast", 0);
```

## 故障排除

### 摄像头无法打开

1. 检查摄像头是否被系统识别：
```bash
ls -l /dev/video*
v4l2-ctl --list-devices
```

2. 检查摄像头权限：
```bash
sudo chmod 666 /dev/video0
```

3. 查看 V4L2 支持的格式：
```bash
v4l2-ctl -d /dev/video0 --list-formats
```

### FFmpeg 库未找到

确保安装了 FFmpeg 开发包：
```bash
# Ubuntu/Debian
sudo apt-get install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev

# Fedora/RHEL
sudo dnf install ffmpeg-devel
```

### 编译错误

1. 确保 CMake 版本 >= 3.14：
```bash
cmake --version
```

2. 清理构建目录并重新编译：
```bash
rm -rf build
mkdir build && cd build
cmake .. -DCCAP_BUILD_EXAMPLES=ON
cmake --build .
```

## 项目结构

```
CameraCapture/
├── examples/
│   └── linux/
│       ├── README.md              # Linux 示例说明
│       ├── CMakeLists.txt         # Linux 示例构建配置
│       ├── linux_camera_list.cpp  # 摄像头列表示例
│       └── video_recorder.cpp     # 视频录制示例
├── src/
│   └── ccap_imp_linux.cpp         # Linux V4L2 实现
├── include/
│   └── ccap.h                     # 主头文件
└── CMakeLists.txt                 # 主构建文件
```

## 扩展功能

### 保存为其他格式

如果你需要保存为其他视频格式（如 AVI、MKV），可以在 FFmpeg 初始化时更改输出格式：

```cpp
AVOutputFormat* output_format = av_guess_format("avi", filename, nullptr);
```

### 添加音频录制

本示例仅支持视频录制。如需添加音频，可以使用 ALSA 或 PulseAudio 库捕获音频，然后使用 FFmpeg 的 AAC 编码器进行编码。

### 实时流

如果你需要实现实时流媒体功能，可以使用 FFmpeg 的 RTMP 输出：

```cpp
AVOutputFormat* output_format = av_guess_format("flv", nullptr, "rtmp");
avformat_alloc_output_context2(&fmt_ctx, output_format, "flv", "rtmp://your-streaming-server/live");
```

## 许可证

本项目基于 MIT 许可证。详情请参阅 [LICENSE](LICENSE) 文件。

## 参考资料

- [ccap 官方仓库](https://github.com/wysaid/CameraCapture)
- [V4L2 官方文档](https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/v4l2.html)
- [FFmpeg 文档](https://ffmpeg.org/documentation.html)
- [V4L2 使用工具](https://wiki.linuxmedia.org/index.php/V4L2_Utilities)

## 技术支持

如果你遇到问题或有疑问，请：

1. 查看 [Issues](https://github.com/ChrisLYF/CameraCapture/issues)
2. 查看 [ccap 官方文档](https://github.com/wysaid/CameraCapture#readme)
3. 参考 V4L2 官方文档

## 更新日志

### 2026-03-28
- 初始版本
- 添加 Linux 视频录制示例
- 添加 CMake 构建支持
- 集成 FFmpeg H.264 编码
- 提供中文使用指南
