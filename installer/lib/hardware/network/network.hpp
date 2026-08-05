#pragma once
#include <string>
#include <vector>

struct NetIfaceInfo {
    std::string name;
    std::string type;
    bool        connected;
    std::string ip_address;
    std::string mac_address;
    std::string gateway;
    std::string dns;
    bool        use_dhcp;
};

struct WifiNetwork {
    std::string ssid;
    int         signal;
    bool        secured;
    bool        connected;
};

class NetworkDetect {
public:
    static std::vector<NetIfaceInfo> get_interfaces();
    static std::vector<WifiNetwork> get_wifi_networks();
};
