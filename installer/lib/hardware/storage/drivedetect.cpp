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
                std::string raw_mp = extract_val(line, "MOUNTPOINT");
                if (raw_mp.rfind("/mnt", 0) == 0 || raw_mp.rfind("/run", 0) == 0 ||
                    raw_mp.rfind("/tmp", 0) == 0 || raw_mp.rfind("/sys", 0) == 0 ||
                    raw_mp.rfind("/proc", 0) == 0) {
                    raw_mp = "";
                }
                p.mount_point = raw_mp;
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

        // Auto-assign smart default target mount points if unassigned
        bool has_root = false;
        bool has_efi = false;
        for (const auto& p : disk_map[name].partitions) {
            if (p.mount_point == "/") has_root = true;
            if (p.mount_point == "/boot/efi") has_efi = true;
        }

        for (auto& p : disk_map[name].partitions) {
            if (p.filesystem == "swap") {
                p.mount_point = "[SWAP]";
            } else if (!has_efi && (p.filesystem == "fat32" || p.filesystem == "vfat") &&
                       (p.flags.find("esp") != std::string::npos || p.flags.find("boot") != std::string::npos ||
                        p.device.find("2") != std::string::npos || p.device.find("1") != std::string::npos)) {
                p.mount_point = "/boot/efi";
                has_efi = true;
            } else if (!has_root && (p.filesystem == "ext4" || p.filesystem == "btrfs" || p.filesystem == "xfs")) {
                p.mount_point = "/";
                has_root = true;
            }
        }

        disks.push_back(disk_map[name]);
    }
    
    return disks;
}
