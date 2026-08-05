#pragma once
#include <string>
#include <vector>

struct GPUInfo {
  std::string name;
  std::string vendor;
  std::string driver_rec;
};

class GPUDetect {
public:
  static std::vector<GPUInfo> get_gpus();
};
