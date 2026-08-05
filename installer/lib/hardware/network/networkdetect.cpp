#include "network.hpp"
#include <cstdio>
#include <array>
#include <memory>
#include <sstream>

std::vector<NetIfaceInfo> NetworkDetect::get_interfaces() {
    std::vector<NetIfaceInfo> ifaces;
    // Basic ip addr parsing
    std::shared_ptr<FILE> pipe(popen("ip addr", "r"), pclose);
    if (!pipe) return ifaces;
    
    std::array<char, 256> buffer;
    NetIfaceInfo current_iface;
    bool in_iface = false;
    
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        std::string line(buffer.data());
        if (!line.empty() && isdigit(line[0])) {
            if (in_iface) {
                ifaces.push_back(current_iface);
            }
            current_iface = NetIfaceInfo();
            in_iface = true;
            
            auto pos_colon = line.find(": ");
            if (pos_colon != std::string::npos) {
                auto next_colon = line.find(":", pos_colon + 2);
                if (next_colon != std::string::npos) {
                    std::string name = line.substr(pos_colon + 2, next_colon - pos_colon - 2);
                    auto at = name.find('@');
                    if (at != std::string::npos) name = name.substr(0, at);
                    current_iface.name = name;
                }
            }
            if (line.find("state UP") != std::string::npos) {
                current_iface.connected = true;
            } else {
                current_iface.connected = false;
            }
            if (current_iface.name.find("wl") == 0 || current_iface.name.find("wlan") == 0) {
                current_iface.type = "Wireless";
            } else if (current_iface.name == "lo") {
                current_iface.type = "Loopback";
            } else {
                current_iface.type = "Ethernet";
            }
            current_iface.use_dhcp = true;
        } else if (in_iface) {
            if (line.find("link/ether") != std::string::npos) {
                auto start = line.find("link/ether") + 11;
                auto end = line.find(' ', start);
                if (end != std::string::npos) {
                    current_iface.mac_address = line.substr(start, end - start);
                }
            } else if (line.find("inet ") != std::string::npos) {
                auto start = line.find("inet ") + 5;
                auto end = line.find('/', start);
                if (end != std::string::npos) {
                    current_iface.ip_address = line.substr(start, end - start);
                }
            }
        }
    }
    if (in_iface) {
        ifaces.push_back(current_iface);
    }
    
    return ifaces;
}

std::vector<WifiNetwork> NetworkDetect::get_wifi_networks() {
    std::vector<WifiNetwork> networks;
    std::shared_ptr<FILE> pipe(popen("nmcli -t -f SSID,SIGNAL,SECURITY,IN-USE dev wifi", "r"), pclose);
    if (!pipe) return networks;

    std::array<char, 256> buffer;
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        std::string line(buffer.data());
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
        if (line.empty()) continue;
        
        // Format: SSID:SIGNAL:SECURITY:IN-USE
        // Since SSID can contain ':', nmcli escapes it in -t mode. This simple parsing might break on escaped colons,
        // but it's a good first approximation.
        std::vector<std::string> parts;
        size_t start = 0;
        size_t end = line.find(':');
        while (end != std::string::npos) {
            // handle simple escaping \:
            if (end > 0 && line[end-1] == '\\') {
                end = line.find(':', end + 1);
                continue;
            }
            parts.push_back(line.substr(start, end - start));
            start = end + 1;
            end = line.find(':', start);
        }
        parts.push_back(line.substr(start));

        if (parts.size() >= 4) {
            WifiNetwork net;
            net.ssid = parts[0];
            if (net.ssid.empty()) continue; // skip hidden for now
            net.signal = std::atoi(parts[1].c_str());
            net.secured = (parts[2] != "");
            net.connected = (parts[3] == "*");
            networks.push_back(net);
        }
    }
    return networks;
}
