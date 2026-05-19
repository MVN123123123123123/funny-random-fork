#pragma once
#include <string>
#include <vector>

struct ServiceOption {
    std::string name;
    std::string description;
    std::string related_package;
};

inline std::vector<ServiceOption> get_default_services() {
    return {
        {"NetworkManager",        "Network connection manager system daemon", "networkmanager"},
        {"bluetooth",             "Bluetooth system service (bluez daemon)",  "bluetooth"},
        {"cups",                  "Common Unix Printing System printing server daemon", "cups"},
        {"power-profiles-daemon", "Power profiles management system service daemon", "power-profiles-daemon"}
    };
}
