# Linux Video Recording Example

This directory contains Linux-specific examples for camera capture and video recording using ccap with V4L2 backend.

## Examples

- [video_recorder](video_recorder.cpp) - Record video from camera and save to MP4/H.264 format

## Requirements

- Linux with V4L2 support (Kernel 2.6+)
- GCC 7+ or Clang 6+
- CMake 3.14+
- FFmpeg development libraries (libavcodec, libavformat, libavutil)

### Installing FFmpeg on Ubuntu/Debian

```bash
sudo apt-get install libavcodec-dev libavformat-dev libavutil-dev
```

### Installing FFmpeg on Fedora/RHEL/CentOS

```bash
sudo dnf install ffmpeg-devel
```

### Installing FFmpeg on Arch Linux

```bash
sudo pacman -S ffmpeg
```

## Building

```bash
cd CameraCapture
mkdir build && cd build
cmake .. -DCCAP_BUILD_EXAMPLES=ON
cmake --build .
```

## Usage

List available cameras:

```bash
./video_recorder --list
```

Record video with default settings:

```bash
./video_recorder -o output.mp4
```

Record with specific resolution and frame rate:

```bash
./video_recorder -o output.mp4 -w 1280 -h 720 -f 30
```

Record for a specific duration (in seconds):

```bash
./video_recorder -o output.mp4 -d 60
```
