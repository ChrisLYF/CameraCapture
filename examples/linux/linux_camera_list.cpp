#include <ccap.h>

#include <iostream>

int main(int argc, char** argv) {
    ccap::Provider camera_provider;
    
    std::cout << "Searching for available cameras..." << std::endl;
    
    auto devices = camera_provider.findDeviceNames();
    
    if (devices.empty()) {
        std::cerr << "No cameras found!" << std::endl;
        return 1;
    }
    
    std::cout << "Found " << devices.size() << " camera(s):" << std::endl;
    std::cout << std::endl;
    
    for (size_t i = 0; i < devices.size(); ++i) {
        std::cout << "Camera " << i << ":" << std::endl;
        std::cout << "  Name: " << devices[i] << std::endl;
        
        ccap::Provider test_provider;
        if (test_provider.open(static_cast<int>(i), false)) {
            auto info = test_provider.getDeviceInfo();
            if (info) {
                std::cout << "  Supported resolutions:" << std::endl;
                for (const auto& resolution : info->supportedResolutions) {
                    std::cout << "    " << resolution.first << "x" << resolution.second << std::endl;
                }
                
                std::cout << "  Supported pixel formats:" << std::endl;
                for (const auto& format : info->supportedPixelFormats) {
                    std::cout << "    " << ccap::pixelFormatToString(format).data() << std::endl;
                }
            }
            test_provider.close();
        }
        std::cout << std::endl;
    }
    
    std::cout << "Use the camera index with other examples." << std::endl;
    std::cout << "Example: ./video_recorder -d 0 -o output.mp4" << std::endl;
    
    return 0;
}
