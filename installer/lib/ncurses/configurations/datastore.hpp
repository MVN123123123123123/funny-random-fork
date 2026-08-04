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

struct RepoSettings {
    bool enable_testing = false;
    bool enable_multilib = true;
    int parallel_downloads = 5;
    bool color = true;
    bool ilovecandy = false;
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
    RepoSettings repo_cfg;

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
    std::string hostname = "fowolinux";

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

    // Fowo Configurations
    std::string fowo_install_mode = "Normal"; // "Normal" or "Master"
    std::string fowo_init_choice = "systemd"; // "systemd", "openrc", "runit", "diy"
    std::string fowo_core_choice = "coreutils"; // "coreutils", "busybox"

    // Fowo Repositories for Master mode
    std::vector<std::pair<std::string, std::string>> fowo_repos = {
        {"kernel", "https://github.com/torvalds/linux.git"},
        {"grub2", "https://git.savannah.gnu.org/git/grub.git"},
        {"util-linux", "https://github.com/util-linux/util-linux.git"},
        {"passwd", "https://github.com/shadow-maint/shadow.git"},
        {"nano", "https://git.savannah.gnu.org/git/nano.git"},
        {"iproute", "https://git.kernel.org/pub/scm/network/iproute2/iproute2.git"},
        {"iputils", "https://github.com/iputils/iputils.git"},
        {"e2fsprogs", "https://git.kernel.org/pub/scm/fs/ext2/e2fsprogs.git"},
        {"dosfstools", "https://github.com/dosfstools/dosfstools.git"},
        {"parted", "https://git.savannah.gnu.org/git/parted.git"},
        {"xfsprogs", "https://git.kernel.org/pub/scm/fs/xfs/xfsprogs-dev.git"},
        {"bash", "https://git.savannah.gnu.org/git/bash.git"},
        {"coreutils", "https://git.savannah.gnu.org/git/coreutils.git"},
        {"busybox", "https://git.busybox.net/busybox.git"},
        {"systemd", "https://github.com/systemd/systemd.git"},
        {"openrc", "https://github.com/OpenRC/openrc.git"},
        {"runit", "https://github.com/g-pape/runit.git"},
        {"m4", "https://github.com/autotools-mirror/m4.git"},
        {"autoconf", "https://git.savannah.gnu.org/git/autoconf.git"},
        {"automake", "https://git.savannah.gnu.org/git/automake.git"},
        {"patch", "https://git.savannah.gnu.org/git/patch.git"},
        {"libtool", "https://git.savannah.gnu.org/git/libtool.git"},
        {"pkg-config", "https://gitlab.freedesktop.org/pkg-config/pkg-config.git"}
    };

    std::vector<std::pair<std::string, std::string>> github_mirrors = {
        {"https://git.savannah.gnu.org/git/autoconf.git", "https://github.com/autotools-mirror/autoconf.git"},
        {"https://git.savannah.gnu.org/git/automake.git", "https://github.com/autotools-mirror/automake.git"},
        {"https://git.savannah.gnu.org/git/libtool.git", "https://github.com/autotools-mirror/libtool.git"},
        {"https://git.savannah.gnu.org/git/coreutils.git", "https://github.com/coreutils/coreutils.git"},
        {"https://git.savannah.gnu.org/git/bash.git", "https://github.com/bminor/bash.git"},
        {"https://git.savannah.gnu.org/git/nano.git", "https://github.com/bminor/nano.git"},
        {"https://git.savannah.gnu.org/git/grub.git", "https://github.com/rhboot/grub2.git"},
        {"https://git.savannah.gnu.org/git/parted.git", "https://github.com/bminor/parted.git"},
        {"https://git.kernel.org/pub/scm/fs/ext2/e2fsprogs.git", "https://github.com/tytso/e2fsprogs.git"},
        {"https://git.busybox.net/busybox.git", "https://github.com/mirror/busybox.git"}
    };

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

