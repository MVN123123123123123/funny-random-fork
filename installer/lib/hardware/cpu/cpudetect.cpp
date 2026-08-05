#include "cpudetect.hpp"
#include <fstream>

std::string CPUDetect::get_cpu_model() {
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    while (std::getline(cpuinfo, line)) {
        if (line.find("model name") == 0) {
            auto pos = line.find(":");
            if (pos != std::string::npos && pos + 1 < line.size()) {
                std::string model = line.substr(pos + 1);
                auto start = model.find_first_not_of(" \t");
                if (start != std::string::npos) model = model.substr(start);
                return model;
            }
        }
    }
    return "Unknown CPU";
}
