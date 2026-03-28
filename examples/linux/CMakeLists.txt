# Linux Examples CMakeLists.txt
# This file is included by the main CMakeLists.txt when CCAP_BUILD_EXAMPLES is enabled

if(NOT UNIX OR APPLE)
    message(STATUS "ccap: Skipping Linux-specific examples (not on Linux)")
    return()
endif()

message(STATUS "ccap: Building Linux-specific examples")

# Check if FFmpeg is available
find_package(PkgConfig)
pkg_check_modules(AVCODEC libavcodec)
pkg_check_modules(AVFORMAT libavformat)
pkg_check_modules(AVUTIL libavutil)
pkg_check_modules(SWSCALE libswscale)

set(FFMPEG_AVAILABLE FALSE)
if(AVCODEC_FOUND AND AVFORMAT_FOUND AND AVUTIL_FOUND AND SWSCALE_FOUND)
    set(FFMPEG_AVAILABLE TRUE)
    message(STATUS "ccap: FFmpeg found, enabling video recorder example")
else()
    message(WARNING "ccap: FFmpeg not found, video recorder example will not be built")
    message(WARNING "ccap: Install FFmpeg development packages to enable video recording:")
    message(WARNING "ccap:   Ubuntu/Debian: sudo apt-get install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev")
    message(WARNING "ccap:   Fedora/RHEL:  sudo dnf install ffmpeg-devel")
    message(WARNING "ccap:   Arch Linux:   sudo pacman -S ffmpeg")
endif()

# Build video recorder example if FFmpeg is available
if(FFMPEG_AVAILABLE)
    add_executable(video_recorder
        video_recorder.cpp
    )
    
    target_link_libraries(video_recorder PRIVATE
        ccap
        ${AVCODEC_LIBRARIES}
        ${AVFORMAT_LIBRARIES}
        ${AVUTIL_LIBRARIES}
        ${SWSCALE_LIBRARIES}
        Threads::Threads
    )
    
    target_include_directories(video_recorder PRIVATE
        ${AVCODEC_INCLUDE_DIRS}
        ${AVFORMAT_INCLUDE_DIRS}
        ${AVUTIL_INCLUDE_DIRS}
        ${SWSCALE_INCLUDE_DIRS}
    )
    
    target_compile_options(video_recorder PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-std=c++17>
    )
    
    install(TARGETS video_recorder
        RUNTIME DESTINATION bin
    )
    
    message(STATUS "ccap: video_recorder example added")
endif()

# Build basic camera list example (no FFmpeg required)
add_executable(linux_camera_list
    linux_camera_list.cpp
)

target_link_libraries(linux_camera_list PRIVATE
    ccap
    Threads::Threads
)

target_compile_options(linux_camera_list PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-std=c++17>
)

install(TARGETS linux_camera_list
    RUNTIME DESTINATION bin
)

message(STATUS "ccap: linux_camera_list example added")
