#pragma once
#include <vector>
#include <string>
#include <cstdint>

// ── Common Data Structures ──────────────────────────────────────────────────

struct LangItem {
    std::string code;
    std::string name;
};

struct KBLayout {
    std::string code;
    std::string name;
};

struct ProfileOption {
    std::string name;
    std::string description;
    std::string default_de;
};

struct MirrorItem {
    std::string url;
    std::string country;
    bool        enabled;
};

struct ReflectorConfig {
    std::string countries = "Worldwide";
    int last_n = 20;
    std::string sort = "rate";
};

struct PacmanSettings {
    bool parallel_downloads = true;
    int max_parallel = 5;
    bool color = true;
    bool check_space = true;
    bool verbose_pkg_lists = true;
};

struct DiskPartition {
    std::string device;
    std::string mount_point;
    uint64_t    size_mb;
    std::string filesystem;
    std::string flags;
    std::string btrfs_subvol;
    std::string btrfs_compress;
};

struct DiskInfo {
    std::string device;
    std::string model;
    uint64_t    size_mb;
    std::string table_type;
    std::vector<DiskPartition> partitions;
};

struct KernelOption {
    std::string package;
    std::string description;
    bool        selected;
};

struct UserAccount {
    std::string username;
    std::string password;
    bool        in_wheel;
};

struct GPUDriverSelection {
    std::string gpu_name;
    std::string driver_package;
    std::string vulkan_package;
};

// ── DataStore Singleton ─────────────────────────────────────────────────────

class DataStore {
public:
    static DataStore& instance() {
        static DataStore inst;
        return inst;
    }

    // Language & Locale
    std::vector<LangItem> languages;
    int selected_language_idx = 0;
    
    std::vector<KBLayout> keyboard_layouts;
    int selected_kb_idx = 0;
    
    std::vector<std::string> locales;
    std::vector<std::string> selected_locales;

    // Mirror Configuration
    std::vector<MirrorItem> mirrors;
    ReflectorConfig reflector_cfg;
    PacmanSettings pacman_cfg;

    // Storage Configuration
    std::vector<DiskInfo> disks;
    int selected_disk_idx = 0;

    // Kernels
    std::vector<KernelOption> kernels;

    // Profiles
    std::vector<ProfileOption> profiles;
    int selected_profile_idx = 0;
    std::string selected_de = "KDE Plasma";
    std::string selected_de_flavor = "Usable";
    std::vector<std::string> server_components;

    // Performance Settings
    bool zram_enabled = false;
    bool zswap_enabled = false;

    // Audio Configuration
    // Options: "PipeWire", "PulseAudio", "None"
    std::string audio_system = "PipeWire";

    // Enabled Systemd Services
    std::vector<std::string> enabled_services = {"NetworkManager", "bluetooth", "cups", "power-profiles-daemon"};

    // ── NEW FIELDS ──────────────────────────────────────────────────────

    // Hostname
    std::string hostname = "archlinux";

    // Bootloader: "GRUB", "systemd-boot", "None"
    std::string bootloader = "GRUB";

    // Accounts
    std::string root_password;
    std::vector<UserAccount> users;

    // Time/Date
    std::string timezone_region = "Asia";
    std::string timezone_city = "Manila";
    bool ntp_enabled = true;
    bool hwclock_sync = true;

    // GPU Driver Selections
    std::vector<GPUDriverSelection> gpu_drivers;

    // Additional user-specified packages
    std::vector<std::string> additional_packages;

    // UEFI detection
    bool is_uefi = false;

    // Helper: get full timezone string
    std::string timezone() const {
        return timezone_region + "/" + timezone_city;
    }

    // Helper: check if a root mountpoint is configured
    bool has_root_mountpoint() const {
        for (const auto& disk : disks) {
            for (const auto& part : disk.partitions) {
                if (part.mount_point == "/") return true;
            }
        }
        return false;
    }

    // Helper: check if EFI partition exists
    bool has_efi_partition() const {
        for (const auto& disk : disks) {
            for (const auto& part : disk.partitions) {
                if (part.mount_point == "/boot/efi" ||
                    part.flags.find("esp") != std::string::npos ||
                    part.flags.find("boot") != std::string::npos) {
                    return true;
                }
            }
        }
        return false;
    }

private:
    DataStore() = default;
    DataStore(const DataStore&) = delete;
    DataStore& operator=(const DataStore&) = delete;
};

