#pragma once
// installer_backend.hpp - Installation command generation and execution
// Generates the sequence of Arch Linux installation commands from DataStore.
#include "../../configurations/datastore.hpp"
#include <string>
#include <vector>
#include <sstream>
#include <functional>
#include <cstdio>
#include <array>

class InstallerBackend {
public:
    using LogCallback = std::function<void(const std::string&)>;

    // Generate the list of installation steps as human-readable descriptions
    static std::vector<std::string> generate_step_descriptions() {
        std::vector<std::string> steps;
        auto& ds = DataStore::instance();

        steps.push_back("Mount partitions");
        steps.push_back("Install base system (pacstrap)");
        steps.push_back("Generate fstab");
        steps.push_back("Set timezone: " + ds.timezone());
        if (ds.hwclock_sync) steps.push_back("Sync hardware clock");
        steps.push_back("Generate locales");
        steps.push_back("Set hostname: " + ds.hostname);
        steps.push_back("Set root password");
        for (const auto& u : ds.users)
            steps.push_back("Create user: " + u.username);
        if (ds.bootloader != "None")
            steps.push_back("Install bootloader: " + ds.bootloader);
        if (!ds.enabled_services.empty())
            steps.push_back("Enable systemd services");
        if (ds.zram_enabled)
            steps.push_back("Configure ZRAM");
        if (ds.audio_system != "None")
            steps.push_back("Install audio: " + ds.audio_system);
        if (!ds.additional_packages.empty())
            steps.push_back("Install additional packages");

        return steps;
    }

    // Generate the actual shell commands for installation
    static std::vector<std::string> generate_commands() {
        std::vector<std::string> cmds;
        auto& ds = DataStore::instance();

        // ── Build pacstrap package list ──
        std::string base_pkgs = "base base-devel";
        
        // Kernels
        for (const auto& k : ds.kernels) {
            if (k.selected) base_pkgs += " " + k.package + " " + k.package + "-headers";
        }

        // Firmware
        base_pkgs += " linux-firmware";

        // Bootloader packages
        if (ds.bootloader == "GRUB") {
            base_pkgs += " grub";
            if (ds.is_uefi) base_pkgs += " efibootmgr";
        }

        // Audio
        if (ds.audio_system == "PipeWire") {
            base_pkgs += " pipewire pipewire-alsa pipewire-pulse pipewire-jack wireplumber";
        } else if (ds.audio_system == "PulseAudio") {
            base_pkgs += " pulseaudio pulseaudio-alsa";
        }

        // GPU drivers
        for (const auto& gpu : ds.gpu_drivers) {
            if (gpu.driver_package != "(none)")
                base_pkgs += " " + gpu.driver_package;
            if (gpu.vulkan_package != "(none)")
                base_pkgs += " " + gpu.vulkan_package;
        }

        // DE/Profile packages
        if (!ds.profiles.empty() && ds.selected_profile_idx < (int)ds.profiles.size()) {
            const auto& profile = ds.profiles[ds.selected_profile_idx];
            if (profile.name == "Desktop") {
                if (ds.selected_de == "KDE Plasma") base_pkgs += " plasma-meta kde-applications-meta sddm";
                else if (ds.selected_de == "Gnome") base_pkgs += " gnome gnome-extra gdm";
                else if (ds.selected_de == "Cinnamon") base_pkgs += " cinnamon lightdm lightdm-gtk-greeter";
                else if (ds.selected_de == "Mate") base_pkgs += " mate mate-extra lightdm lightdm-gtk-greeter";
                else if (ds.selected_de == "XFCE") base_pkgs += " xfce4 xfce4-goodies lightdm lightdm-gtk-greeter";
            }
        }

        // Network
        base_pkgs += " networkmanager";

        // Additional packages
        for (const auto& pkg : ds.additional_packages) {
            base_pkgs += " " + pkg;
        }

        // ZRAM
        if (ds.zram_enabled) {
            base_pkgs += " zram-generator";
        }

        // ── Mount partitions ──
        // Find root partition
        std::string root_device;
        for (const auto& disk : ds.disks) {
            for (const auto& part : disk.partitions) {
                if (part.mount_point == "/") {
                    root_device = part.device;
                    if (root_device.find("/dev/") != 0) root_device = "/dev/" + root_device;
                    cmds.push_back("mount " + root_device + " /mnt");
                }
            }
        }

        // Mount other partitions
        for (const auto& disk : ds.disks) {
            for (const auto& part : disk.partitions) {
                if (part.mount_point != "/" && part.mount_point != "[SWAP]" && !part.mount_point.empty()) {
                    std::string dev = part.device;
                    if (dev.find("/dev/") != 0) dev = "/dev/" + dev;
                    cmds.push_back("mkdir -p /mnt" + part.mount_point);
                    cmds.push_back("mount " + dev + " /mnt" + part.mount_point);
                }
                if (part.mount_point == "[SWAP]" || part.filesystem == "swap") {
                    std::string dev = part.device;
                    if (dev.find("/dev/") != 0) dev = "/dev/" + dev;
                    cmds.push_back("swapon " + dev);
                }
            }
        }

        // ── Pacstrap ──
        cmds.push_back("pacstrap -K /mnt " + base_pkgs);

        // ── Fstab ──
        cmds.push_back("genfstab -U /mnt >> /mnt/etc/fstab");

        // ── arch-chroot commands ──
        auto chroot_cmd = [](const std::string& cmd) {
            return "arch-chroot /mnt /bin/bash -c '" + cmd + "'";
        };

        // Timezone
        cmds.push_back(chroot_cmd("ln -sf /usr/share/zoneinfo/" + ds.timezone() + " /etc/localtime"));
        if (ds.hwclock_sync)
            cmds.push_back(chroot_cmd("hwclock --systohc"));

        // Locale
        for (const auto& loc : ds.selected_locales) {
            cmds.push_back(chroot_cmd("sed -i \"s/#" + loc + "/" + loc + "/\" /etc/locale.gen"));
        }
        cmds.push_back(chroot_cmd("locale-gen"));
        if (!ds.selected_locales.empty()) {
            cmds.push_back(chroot_cmd("echo LANG=" + ds.selected_locales[0] + " > /etc/locale.conf"));
        }

        // Keyboard
        if (ds.selected_kb_idx < (int)ds.keyboard_layouts.size()) {
            cmds.push_back(chroot_cmd("echo KEYMAP=" + ds.keyboard_layouts[ds.selected_kb_idx].code + " > /etc/vconsole.conf"));
        }

        // Hostname
        cmds.push_back(chroot_cmd("echo " + ds.hostname + " > /etc/hostname"));
        cmds.push_back(chroot_cmd("echo '127.0.0.1 localhost' >> /etc/hosts"));
        cmds.push_back(chroot_cmd("echo '::1       localhost' >> /etc/hosts"));
        cmds.push_back(chroot_cmd("echo '127.0.1.1 " + ds.hostname + "' >> /etc/hosts"));

        // Root password
        if (!ds.root_password.empty()) {
            cmds.push_back(chroot_cmd("echo 'root:" + ds.root_password + "' | chpasswd"));
        }

        // User accounts
        for (const auto& u : ds.users) {
            std::string useradd = "useradd -m";
            if (u.in_wheel) useradd += " -G wheel";
            useradd += " " + u.username;
            cmds.push_back(chroot_cmd(useradd));
            cmds.push_back(chroot_cmd("echo '" + u.username + ":" + u.password + "' | chpasswd"));
        }
        // Enable sudo for wheel
        if (!ds.users.empty()) {
            cmds.push_back(chroot_cmd("sed -i 's/# %wheel ALL=(ALL:ALL) ALL/%wheel ALL=(ALL:ALL) ALL/' /etc/sudoers"));
        }

        // Bootloader
        if (ds.bootloader == "GRUB") {
            if (ds.is_uefi) {
                cmds.push_back(chroot_cmd("grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=GRUB"));
            } else {
                // For BIOS, install to the disk device
                if (!root_device.empty()) {
                    // Strip partition number to get disk device
                    std::string disk_dev = root_device;
                    while (!disk_dev.empty() && isdigit(disk_dev.back())) disk_dev.pop_back();
                    // Handle nvme (strip trailing 'p')
                    if (!disk_dev.empty() && disk_dev.back() == 'p' && disk_dev.find("nvme") != std::string::npos)
                        disk_dev.pop_back();
                    cmds.push_back(chroot_cmd("grub-install --target=i386-pc " + disk_dev));
                }
            }
            cmds.push_back(chroot_cmd("grub-mkconfig -o /boot/grub/grub.cfg"));
        } else if (ds.bootloader == "systemd-boot") {
            cmds.push_back(chroot_cmd("bootctl install"));
        }

        // Enable services
        for (const auto& svc : ds.enabled_services) {
            cmds.push_back(chroot_cmd("systemctl enable " + svc));
        }

        // NTP
        if (ds.ntp_enabled) {
            cmds.push_back(chroot_cmd("systemctl enable systemd-timesyncd"));
        }

        // Display manager
        if (!ds.profiles.empty() && ds.selected_profile_idx < (int)ds.profiles.size()) {
            const auto& profile = ds.profiles[ds.selected_profile_idx];
            if (profile.name == "Desktop") {
                if (ds.selected_de == "KDE Plasma") cmds.push_back(chroot_cmd("systemctl enable sddm"));
                else if (ds.selected_de == "Gnome") cmds.push_back(chroot_cmd("systemctl enable gdm"));
                else cmds.push_back(chroot_cmd("systemctl enable lightdm"));
            }
        }

        // ZRAM config
        if (ds.zram_enabled) {
            cmds.push_back(chroot_cmd("mkdir -p /etc/systemd/zram-generator.conf.d"));
            cmds.push_back(chroot_cmd("echo '[zram0]' > /etc/systemd/zram-generator.conf"));
            cmds.push_back(chroot_cmd("echo 'zram-size = ram / 2' >> /etc/systemd/zram-generator.conf"));
            cmds.push_back(chroot_cmd("echo 'compression-algorithm = zstd' >> /etc/systemd/zram-generator.conf"));
        }

        // Unmount
        cmds.push_back("umount -R /mnt");

        return cmds;
    }

    // Execute a single command, logging output
    static int execute_command(const std::string& cmd, LogCallback log) {
        log("[CMD] " + cmd);
        
        std::string full_cmd = cmd + " 2>&1";
        FILE* pipe = popen(full_cmd.c_str(), "r");
        if (!pipe) {
            log("[ERROR] Failed to execute command");
            return -1;
        }

        std::array<char, 256> buffer;
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            std::string line(buffer.data());
            // Strip trailing newline
            while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
                line.pop_back();
            if (!line.empty())
                log("  " + line);
        }

        int ret = pclose(pipe);
        if (ret != 0) {
            log("[WARN] Command exited with code " + std::to_string(ret));
        }
        return ret;
    }

    // Generate a summary string for the confirmation dialog
    static std::string generate_summary() {
        auto& ds = DataStore::instance();
        std::ostringstream ss;

        ss << "Hostname:    " << ds.hostname << "\n";
        ss << "Timezone:    " << ds.timezone() << "\n";
        ss << "Bootloader:  " << ds.bootloader << "\n";
        ss << "Boot Mode:   " << (ds.is_uefi ? "UEFI" : "BIOS/Legacy") << "\n";
        ss << "Audio:       " << ds.audio_system << "\n";
        
        ss << "Kernels:     ";
        bool first = true;
        for (const auto& k : ds.kernels) {
            if (k.selected) {
                if (!first) ss << ", ";
                ss << k.package;
                first = false;
            }
        }
        ss << "\n";

        ss << "Root Pass:   " << (ds.root_password.empty() ? "NOT SET" : "Set") << "\n";
        ss << "Users:       " << ds.users.size() << "\n";

        if (!ds.profiles.empty()) {
            ss << "Profile:     " << ds.profiles[ds.selected_profile_idx].name;
            if (ds.profiles[ds.selected_profile_idx].name == "Desktop")
                ss << " (" << ds.selected_de << " / " << ds.selected_de_flavor << ")";
            ss << "\n";
        }

        ss << "ZRAM:        " << (ds.zram_enabled ? "ON" : "OFF") << "\n";
        ss << "ZSWAP:       " << (ds.zswap_enabled ? "ON" : "OFF") << "\n";
        ss << "NTP:         " << (ds.ntp_enabled ? "ON" : "OFF") << "\n";
        ss << "Extra Pkgs:  " << ds.additional_packages.size() << "\n";

        return ss.str();
    }
};
