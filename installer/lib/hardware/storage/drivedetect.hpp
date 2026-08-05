#pragma once
#include <vector>
#include "../../ncurses/configurations/datastore.hpp"

class StorageDetect {
public:
  static std::vector<DiskInfo> get_disks();
};
