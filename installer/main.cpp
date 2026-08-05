// main.cpp - Entry point for HarukaInstaller (Production)
// This is the production entry point. In a real deployment, this would
// detect real hardware. For now, it uses simulation data as a placeholder
// but includes proper UEFI detection and production gating.

#include "lib/ncurses/configurations/config_defaults.hpp"
#include "lib/hardware/cpu/cpudetect.hpp"
#include "lib/hardware/gpu/gpudetect.hpp"
#include "lib/hardware/network/network.hpp"
#include "lib/hardware/storage/drivedetect.hpp"
#include "lib/ncurses/ncurseslib.hpp"
#include "lib/ncurses/mainmenu/mainmenu.hpp"
#include "lib/ncurses/configurations/datastore.hpp"

// Header for get_regions

#include <vector>
#include <string>
#include <sys/stat.h>

// ── UEFI detection ──────────────────────────────────────────────────────────
static bool detect_uefi() {
    struct stat st;
    return (stat("/sys/firmware/efi", &st) == 0 && S_ISDIR(st.st_mode));
}

int main() {
    // ── Detect boot mode ────────────────────────────────────────────────
    bool is_uefi = detect_uefi();

    // ── Load data ───────────────────────────────────────────────────────
    auto sim_langs     = ConfigDefaults::get_languages();
    auto sim_mirrors   = ConfigDefaults::get_mirrors();
    auto sim_kblayouts = ConfigDefaults::get_keyboard_layouts();
    auto sim_locales   = ConfigDefaults::get_locales();
    auto sim_disks     = StorageDetect::get_disks();
    auto sim_networks  = NetworkDetect::get_interfaces();
    auto sim_wifi      = NetworkDetect::get_wifi_networks();
    auto sim_kernels   = ConfigDefaults::get_kernels();
    auto sim_gpus      = GPUDetect::get_gpus();
    auto sim_profiles  = ConfigDefaults::get_profiles();

    // ── Populate DataStore ──────────────────────────────────────────────
    auto& ds = DataStore::instance();

    // Set UEFI mode
    ds.is_uefi = is_uefi;
    
    for (auto& l : sim_langs)
        ds.languages.push_back({l.code, l.name});
        
    for (auto& m : sim_mirrors)
        ds.mirrors.push_back({m.url, m.country, m.enabled});
        
    for (auto& k : sim_kblayouts)
        ds.keyboard_layouts.push_back({k.code, k.name});
        
    ds.locales = sim_locales;
        
    for (auto& d : sim_disks) {
        DiskInfo di;
        di.device = d.device;
        di.model = d.model;
        di.size_mb = d.size_mb;
        di.table_type = d.table_type;
        for (auto& p : d.partitions)
            di.partitions.push_back({p.device, p.mount_point, p.size_mb, p.filesystem, p.flags, "", ""});
        ds.disks.push_back(di);
    }
    
    for (auto& k : sim_kernels)
        ds.kernels.push_back({k.package, k.description, false});
    if (!ds.kernels.empty()) ds.kernels[0].selected = true;

    std::vector<NetIfaceInfo> net_items;
    for (auto& n : sim_networks)
        net_items.push_back({n.name, n.type, n.connected, n.ip_address,
                             n.mac_address, n.gateway, n.dns, n.use_dhcp});

    std::vector<WifiNetwork> wifi_items;
    for (auto& w : sim_wifi)
        wifi_items.push_back({w.ssid, w.signal, w.secured, w.connected});

    std::vector<GPUInfo> gpu_items;
    for (auto& g : sim_gpus)
        gpu_items.push_back({g.name, g.vendor, g.driver_rec});

    for (auto& p : sim_profiles)
        ds.profiles.push_back({p.name, p.description, p.default_de});

    // ── Initialize ncurses ──────────────────────────────────────────────
    NcursesLib ncurses;
    ncurses.init_ncurses();

    // ── Build main menu using centralized page setup ────────────────────
    // Fall back to get_regions() database from regions.hpp automatically
    MainMenu menu(gpu_items, net_items, wifi_items, {});

    // ── Run ─────────────────────────────────────────────────────────────
    menu.run();

    // ── Cleanup ─────────────────────────────────────────────────────────
    ncurses.end_ncurses();
    return 0;
}
