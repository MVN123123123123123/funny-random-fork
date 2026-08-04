#pragma once
// simulation_test.hpp - Mock data for testing HarukaInstaller TUI
// This provides simulated hardware/config data so the UI can be tested
// without real hardware detection or system modifications.

#include <string>
#include <vector>
#include <cstdint>

namespace SimData {

// ── Disk / Partition Simulation ─────────────────────────────────────────────

struct Partition {
    std::string device;       // "sda1"
    std::string mount_point;  // "/boot"
    uint64_t    size_mb;      // size in MB
    std::string filesystem;   // "fat32", "ext4", "btrfs", "swap", "xfs", "ntfs"
    std::string flags;        // "boot,esp", "", "swap"
};

struct Disk {
    std::string device;       // "/dev/sda"
    std::string model;        // "Samsung SSD 860 EVO 120GB"
    uint64_t    size_mb;      // total size in MB
    std::string table_type;   // "gpt", "mbr", "" (no table)
    std::vector<Partition> partitions;
};

inline std::vector<Disk> get_disks() {
    return {
        {
            "/dev/sda", "Virtual Disk", 51200, "gpt",
            {
                {"sda1", "/boot/efi", 512,   "fat32", "boot,esp"},
                {"sda2", "/",         46688, "ext4",  ""},
                {"sda3", "[SWAP]",    4000,  "swap",  "swap"},
            }
        },
    };
}

// ── Network Interface Simulation ────────────────────────────────────────────

struct NetworkInterface {
    std::string name;        // "eth0"
    std::string type;        // "Ethernet" or "Wireless"
    bool        connected;
    std::string ip_address;
    std::string mac_address;
    std::string gateway;
    std::string dns;
    bool        use_dhcp;
};

inline std::vector<NetworkInterface> get_network_interfaces() {
    return {
        {"eth0",  "Ethernet", true,  "192.168.1.100", "AA:BB:CC:DD:EE:01", "192.168.1.1", "1.1.1.1", true},
        {"lo",    "Loopback", true,  "127.0.0.1",     "00:00:00:00:00:00", "",            "",        true},
    };
}

// ── WiFi Network Simulation ─────────────────────────────────────────────────

struct WifiNet {
    std::string ssid;
    int         signal;   // 0-100
    bool        secured;
    bool        connected;
};

inline std::vector<WifiNet> get_wifi_networks() {
    return {};
}

// ── Language List ───────────────────────────────────────────────────────────

struct Language {
    std::string code;   // "en"
    std::string name;   // "English"
};

inline std::vector<Language> get_languages() {
    return {
        {"en", "English"},
        {"ja", "日本語 (Japanese)"},
        {"de", "Deutsch (German)"},
        {"fr", "Français (French)"},
        {"es", "Español (Spanish)"},
        {"pt", "Português (Portuguese)"},
        {"it", "Italiano (Italian)"},
        {"ko", "한국어 (Korean)"},
        {"zh", "中文 (Chinese)"},
        {"ru", "Русский (Russian)"},
        {"ar", "العربية (Arabic)"},
        {"pl", "Polski (Polish)"},
        {"nl", "Nederlands (Dutch)"},
        {"sv", "Svenska (Swedish)"},
        {"fi", "Suomi (Finnish)"},
        {"tl", "Tagalog (Filipino)"},
    };
}

// ── Keyboard Layouts ────────────────────────────────────────────────────────

struct KeyboardLayout {
    std::string code;   // "us"
    std::string name;   // "English (US)"
};

inline std::vector<KeyboardLayout> get_keyboard_layouts() {
    return {
        {"us",      "English (US)"},
        {"gb",      "English (UK)"},
        {"de",      "German"},
        {"fr",      "French"},
        {"es",      "Spanish"},
        {"it",      "Italian"},
        {"jp",      "Japanese"},
        {"kr",      "Korean"},
        {"br",      "Portuguese (Brazil)"},
        {"ru",      "Russian"},
        {"dvorak",  "Dvorak"},
        {"colemak", "Colemak"},
    };
}

// ── Locales ─────────────────────────────────────────────────────────────────

inline std::vector<std::string> get_locales() {
    return {
        "en_US.UTF-8",
        "en_GB.UTF-8",
        "ja_JP.UTF-8",
        "de_DE.UTF-8",
        "fr_FR.UTF-8",
        "es_ES.UTF-8",
        "it_IT.UTF-8",
        "ko_KR.UTF-8",
        "zh_CN.UTF-8",
        "zh_TW.UTF-8",
        "pt_BR.UTF-8",
        "ru_RU.UTF-8",
        "pl_PL.UTF-8",
        "nl_NL.UTF-8",
        "sv_SE.UTF-8",
        "fi_FI.UTF-8",
    };
}

// ── Kernel List ─────────────────────────────────────────────────────────────

struct Kernel {
    std::string package;      // "linux"
    std::string description;  // "The default Linux kernel"
};

inline std::vector<Kernel> get_kernels() {
    return {
        {"linux",          "Default Linux kernel and modules"},
    };
}

// ── Timezone Regions & Cities ───────────────────────────────────────────────

struct TimezoneRegion {
    std::string region;
    std::vector<std::string> cities;
};

inline std::vector<TimezoneRegion> get_timezones() {
    return {
        {"Asia",    {"Manila", "Tokyo", "Seoul", "Shanghai", "Kolkata", "Singapore", "Bangkok", "Dubai"}},
        {"Europe",  {"London", "Berlin", "Paris", "Rome", "Madrid", "Moscow", "Amsterdam", "Stockholm"}},
        {"America", {"New_York", "Chicago", "Denver", "Los_Angeles", "Sao_Paulo", "Toronto", "Mexico_City"}},
        {"Pacific", {"Auckland", "Sydney", "Fiji", "Honolulu"}},
        {"Africa",  {"Cairo", "Johannesburg", "Lagos", "Nairobi"}},
    };
}

// ── Mirror List ─────────────────────────────────────────────────────────────

struct Mirror {
    std::string url;
    std::string country;
    bool        enabled;
};

inline std::vector<Mirror> get_mirrors() {
    return {
        {"https://mirror.fowo.org/repo/$repo/os/$arch",            "Worldwide",   true},
        {"https://us.mirror.fowo.org/repo/$repo/os/$arch",         "United States", true},
        {"https://jp.mirror.fowo.org/repo/$repo/os/$arch",         "Japan",       false},
        {"https://eu.mirror.fowo.org/repo/$repo/os/$arch",         "Germany",     true},
    };
}

// ── GPU Detection Simulation ────────────────────────────────────────────────

struct GPU {
    std::string name;
    std::string vendor;    // "NVIDIA", "AMD", "Intel"
    std::string driver_rec; // recommended driver package
};

inline std::vector<GPU> get_gpus() {
    return {
        {"Virtio GPU", "Virtual", ""},
    };
}

// ── Profile Options ─────────────────────────────────────────────────────────

struct Profile {
    std::string name;
    std::string description;
    std::string default_de;  // default desktop env, empty for server/minimal
};

inline std::vector<Profile> get_profiles() {
    return {
        {"Desktop",    "Full desktop environment with GUI applications", "KDE Plasma"},
        {"Server",     "Headless server with essential tools only",      ""},
        {"Minimalist", "Bare minimum base system, manually configure",   ""},
    };
}

} // namespace SimData
