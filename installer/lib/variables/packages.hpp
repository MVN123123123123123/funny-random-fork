#pragma once
#include "../configurations/datastore.hpp"
#include <string>

inline bool is_package_installed(const std::string& pkg_name) {
    auto& ds = DataStore::instance();
    std::string profile = ds.profiles.empty() ? "" : ds.profiles[ds.selected_profile_idx].name;
    
    if (profile == "Desktop") {
        if (pkg_name == "networkmanager") {
            // NetworkManager is always installed on Desktop
            return true;
        }
        if (pkg_name == "bluez" || pkg_name == "bluez-utils" || pkg_name == "bluetooth") {
            // Bluetooth is installed in Usable and Full flavors
            return ds.selected_de_flavor == "Usable" || ds.selected_de_flavor == "Full";
        }
        if (pkg_name == "cups") {
            // CUPS is installed in Usable and Full flavors
            return ds.selected_de_flavor == "Usable" || ds.selected_de_flavor == "Full";
        }
        if (pkg_name == "power-profiles-daemon" || pkg_name == "power-profiles" || pkg_name == "power") {
            // Power management is installed in Full flavor
            return ds.selected_de_flavor == "Full";
        }
    }
    else if (profile == "Server") {
        // NetworkManager is available on Server profiles
        if (pkg_name == "networkmanager") return true;
    }
    
    return false;
}
