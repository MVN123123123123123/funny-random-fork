#include "drivedetect.hpp"
#include <cstdio>
#include <array>
#include <memory>
#include <map>

// Helper to extract a value from the lsblk -P output
static std::string extract_val(const std::string& line, const std::string& key) {
    std::string search = key + "=\"";
    auto start = line.find(search);
    if (start != std::string::npos) {
        start += search.length();
        auto end = line.find("\"", start);
        if (end != std::string::npos) {
            return line.substr(start, end - start);
        }
    }
    return "";
}

std::vector<DiskInfo> StorageDetect::get_disks() {
    std::vector<DiskInfo> disks;
    std::shared_ptr<FILE> pipe(popen("lsblk -P -b -o NAME,MODEL,SIZE,TYPE,FSTYPE,MOUNTPOINT,PARTTYPE,PKNAME", "r"), pclose);
    if (!pipe) return disks;

    std::map<std::string, DiskInfo> disk_map;
    std::vector<std::string> disk_order;

    std::array<char, 512> buffer;
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        std::string line(buffer.data());
        std::string type = extract_val(line, "TYPE");
        std::string name = extract_val(line, "NAME");
        
        if (type == "disk") {
            DiskInfo d;
            d.device = "/dev/" + name;
            d.model = extract_val(line, "MODEL");
            std::string size_str = extract_val(line, "SIZE");
            d.size_mb = size_str.empty() ? 0 : std::stoull(size_str) / (1024 * 1024);
            d.table_type = ""; // lsblk doesn't easily show PTType without root in some cases, or we can use blkid. For now, empty or basic assumption.
            
            disk_map[name] = d;
            disk_order.push_back(name);
        } else if (type == "part") {
            std::string pkname = extract_val(line, "PKNAME");
            if (disk_map.find(pkname) != disk_map.end()) {
                DiskPartition p;
                p.device = "/dev/" + name;
                p.mount_point = extract_val(line, "MOUNTPOINT");
                std::string size_str = extract_val(line, "SIZE");
                p.size_mb = size_str.empty() ? 0 : std::stoull(size_str) / (1024 * 1024);
                p.filesystem = extract_val(line, "FSTYPE");
                p.flags = extract_val(line, "PARTTYPE"); // GUID or hex code
                
                disk_map[pkname].partitions.push_back(p);
            }
        }
    }
    
    for (const auto& name : disk_order) {
        if (name.find("zram") != std::string::npos || name.find("loop") != std::string::npos) {
            continue; // Ignore zram and loop devices
        }
        disks.push_back(disk_map[name]);
    }
    
    return disks;
}
