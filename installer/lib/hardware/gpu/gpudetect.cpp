#include "gpudetect.hpp"
#include <cstdio>
#include <array>
#include <memory>

std::vector<GPUInfo> GPUDetect::get_gpus() {
    std::vector<GPUInfo> gpus;
    std::shared_ptr<FILE> pipe(popen("lspci -mm", "r"), pclose);
    if (!pipe) return gpus;
    
    std::array<char, 256> buffer;
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        std::string line(buffer.data());
        if (line.find("VGA compatible controller") != std::string::npos ||
            line.find("3D controller") != std::string::npos) {
            
            GPUInfo gpu;
            auto pos1 = line.find('"', line.find('"', line.find("controller") + 1) + 1);
            if (pos1 != std::string::npos) {
                auto start_vendor = line.find('"', pos1 + 1);
                if (start_vendor != std::string::npos) {
                    auto end_vendor = line.find('"', start_vendor + 1);
                    if (end_vendor != std::string::npos) {
                        gpu.vendor = line.substr(start_vendor + 1, end_vendor - start_vendor - 1);
                        
                        auto start_name = line.find('"', end_vendor + 1);
                        if (start_name != std::string::npos) {
                            auto end_name = line.find('"', start_name + 1);
                            if (end_name != std::string::npos) {
                                gpu.name = line.substr(start_name + 1, end_name - start_name - 1);
                            }
                        }
                    }
                }
            }
            if (gpu.name.empty()) gpu.name = "Unknown GPU";
            if (gpu.vendor.empty()) gpu.vendor = "Unknown Vendor";
            gpu.driver_rec = "";
            gpus.push_back(gpu);
        }
    }
    return gpus;
}
