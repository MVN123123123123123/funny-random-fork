#pragma once
// installer_backend.hpp - Installation command generation and execution
// Generates the sequence of Fowo Linux installation commands from DataStore.
#include "../../configurations/datastore.hpp"
#include <string>
#include <vector>
#include <sstream>
#include <functional>
#include <cstdio>
#include <array>

class InstallerBackend {
public:
    static std::string escape_shell_arg(const std::string& arg) {
        std::string escaped = "'";
        for (char c : arg) {
            if (c == '\'') escaped += "'\\''";
            else escaped += c;
        }
        escaped += "'";
        return escaped;
    }

    using LogCallback = std::function<void(const std::string&)>;

    // Generate the list of installation steps as human-readable descriptions
    static std::vector<std::string> generate_step_descriptions() {
        std::vector<std::string> steps;
        auto& ds = DataStore::instance();

        steps.push_back("Mount partitions");
        steps.push_back("Bootstrap Fowo (" + ds.fowo_install_mode + " Mode)");
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

        // ── Create and Format partitions ──
        for (const auto& disk : ds.disks) {
            for (const auto& part : disk.partitions) {
                if (!part.mount_point.empty()) {
                    std::string dev = part.device;
                    if (dev.find("/dev/") != 0) dev = "/dev/" + dev;
                    
                    // Simple partition creation if device does not exist
                    cmds.push_back("if [ ! -b " + dev + " ]; then parted -s " + disk.device + " mkpart primary " + part.filesystem + " 0% " + std::to_string(part.size_mb) + "MB || true; sleep 2; fi");
                    
                    if (part.filesystem == "ext4") {
                        cmds.push_back("mkfs.ext4 -F " + dev + " || true");
                    } else if (part.filesystem == "btrfs") {
                        cmds.push_back("mkfs.btrfs -f " + dev + " || true");
                    } else if (part.filesystem == "xfs") {
                        cmds.push_back("mkfs.xfs -f " + dev + " || true");
                    } else if (part.filesystem == "fat32" || part.filesystem == "vfat") {
                        cmds.push_back("mkfs.vfat -F 32 " + dev + " || true");
                    } else if (part.filesystem == "swap" || part.mount_point == "[SWAP]") {
                        cmds.push_back("mkswap -f " + dev + " || true");
                    }
                }
            }
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

        // ── Build package list for Base ──
        std::string base_pkgs = "kernel grub2-pc grub2-efi-x64 util-linux passwd nano iproute iputils e2fsprogs dosfstools parted xfsprogs";
        for (const auto& pkg : ds.additional_packages) {
            base_pkgs += " " + pkg;
        }

        // DE/Profile packages
        std::string de_pkgs = "";
        if (ds.selected_de == "KDE Plasma") de_pkgs = "@kde-desktop-environment";
        else if (ds.selected_de == "GNOME") de_pkgs = "@gnome-desktop";
        else if (ds.selected_de == "XFCE") de_pkgs = "@xfce-desktop-environment";
        
        for (const auto& svc : ds.server_components) {
            if (svc == "OpenSSH") de_pkgs += " openssh-server";
            else if (svc == "Docker") de_pkgs += " docker";
            else if (svc == "Nginx") de_pkgs += " nginx";
        }
        if (!de_pkgs.empty()) base_pkgs += " " + de_pkgs;

        // Audio System
        if (ds.audio_system == "PipeWire") {
            base_pkgs += " pipewire pipewire-pulseaudio pipewire-alsa pipewire-jack wireplumber";
        } else if (ds.audio_system == "PulseAudio") {
            base_pkgs += " pulseaudio pulseaudio-utils";
        }

        // ZRAM
        if (ds.zram_enabled) {
            base_pkgs += " zram-generator";
        }

        // GPU drivers
        for (const auto& gpu : ds.gpu_drivers) {
            if (gpu.driver_package != "(none)" && !gpu.driver_package.empty()) base_pkgs += " " + gpu.driver_package;
            if (gpu.vulkan_package != "(none)" && !gpu.vulkan_package.empty()) base_pkgs += " " + gpu.vulkan_package;
        }

        // ── Bootstrap Fowo ──
        if (ds.fowo_install_mode == "Normal") {
            cmds.push_back("dnf --use-host-config --installroot=/mnt --releasever=45 install -y @core systemd dnf " + base_pkgs);
            cmds.push_back("mkdir -p /mnt/usr/local/bin");
            cmds.push_back("cp /usr/local/bin/fowo /mnt/usr/local/bin/ || true");
        } else {
            // Master mode uses fowo to build from source mirrors
            cmds.push_back("mkdir -p /mnt/tmp");
            std::string script = "cat << 'EOF' > /mnt/tmp/install_master.sh\n";
            script += "#!/bin/bash\n";
            script += "set -e\n";
            script += "declare -A FOWO_REPOS=(\n";
            for (const auto& r : ds.fowo_repos) {
                script += "  [\"" + r.first + "\"]=\"" + r.second + "\"\n";
            }
            script += ")\n";
            script += "declare -A GITHUB_MIRRORS=(\n";
            for (const auto& m : ds.github_mirrors) {
                script += "  [\"" + m.first + "\"]=\"" + m.second + "\"\n";
            }
            script += ")\n";
            
            script += R"(
echo "Ranking source mirror speeds..."
declare -A _host_url
for _pkg in "${!FOWO_REPOS[@]}"; do
    _url="${FOWO_REPOS[$_pkg]}"
    _host="${_url#https://}"
    _host="${_host%%/*}"
    [[ -n "${_host_url[$_host]+x}" ]] || _host_url[$_host]="$_url"
done
[[ -n "${_host_url[github.com]+x}" ]] || _host_url["github.com"]="https://github.com/autotools-mirror/m4.git"

declare -A HOST_SPEED
_mdir=$(mktemp -d)
for _host in "${!_host_url[@]}"; do
    _shost="${_host//[\/.]/_}"
    (
        _b=$(date +%s)
        if timeout 8 git ls-remote "${_host_url[$_host]}" HEAD >/dev/null 2>&1; then
            _a=$(date +%s)
            echo $((_a - _b))
        else
            echo 999
        fi
    ) > "$_mdir/$_shost" 2>/dev/null &
done
wait
for _host in "${!_host_url[@]}"; do
    _shost="${_host//[\/.]/_}"
    HOST_SPEED[$_host]=$(cat "$_mdir/$_shost" 2>/dev/null || echo 999)
done
rm -rf "$_mdir"

_switched=0
for _pkg in "${!FOWO_REPOS[@]}"; do
    _url="${FOWO_REPOS[$_pkg]}"
    _host="${_url#https://}"
    _host="${_host%%/*}"
    if [[ ${HOST_SPEED[$_host]:-0} -ge 3 ]] && [[ -n "${GITHUB_MIRRORS[$_url]+x}" ]]; then
        FOWO_REPOS[$_pkg]="${GITHUB_MIRRORS[$_url]}"
        _switched=1
    fi
done

mkdir -p /etc/fowo
cat > /etc/fowo/config << CONFIG_EOF
FLAGS_util-linux=-Dbuild-python=disabled
ALIAS ${FOWO_REPOS[patch]} = patch
ALIAS ${FOWO_REPOS[m4]} = m4
ALIAS ${FOWO_REPOS[autoconf]} = autoconf
ALIAS ${FOWO_REPOS[automake]} = automake
ALIAS ${FOWO_REPOS[libtool]} = libtool
ALIAS ${FOWO_REPOS[pkg-config]} = pkg-config
CONFIG_EOF

)";
            std::string pkgs = "patch m4 autoconf automake libtool pkg-config " + base_pkgs;
            if (ds.fowo_core_choice == "busybox") pkgs += " busybox";
            else pkgs += " bash coreutils";
            
            if (ds.fowo_init_choice == "systemd") pkgs += " systemd";
            else if (ds.fowo_init_choice == "openrc") pkgs += " openrc";
            else if (ds.fowo_init_choice == "runit") pkgs += " runit";

            script += "SELECTED_PKGS=(" + pkgs + ")\n";
            script += R"(
export FOWO_ROOT="/"
export PATH="/usr/local/bin:/usr/bin:/bin:$PATH"
for pkg in "${SELECTED_PKGS[@]}"; do
    fowo install --no-edit "${FOWO_REPOS[$pkg]}"
done
rm -rf /tmp/fowo_build /tmp/fowo_dest_stage 2>/dev/null || true

mkdir -p /etc
cat > /etc/os-release << 'OS_EOF'
NAME="FeOwOra"
PRETTY_NAME="FeOwOra Linux"
ID=feowora
ID_LIKE=fedora
VERSION_ID="1.0"
HOME_URL="https://github.com/FeOwOra"
OS_EOF
)";

            if (ds.fowo_init_choice == "diy") {
                script += R"(
mkdir -p /sbin
cat > /sbin/init << 'INIT_EOF'
#!/bin/sh
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev
echo "Welcome to FeOwOra DIY Init!"
exec /bin/sh
INIT_EOF
chmod +x /sbin/init
)";
            }

            script += "EOF\n";
            cmds.push_back(script);
            cmds.push_back("chmod +x /mnt/tmp/install_master.sh");

            // Prepare chroot environment to run the master script
            cmds.push_back("mount -o bind /dev /mnt/dev");
            cmds.push_back("mount -t proc proc /mnt/proc");
            cmds.push_back("mount -t sysfs sysfs /mnt/sys");
            cmds.push_back("mount -t tmpfs tmpfs /mnt/run");
            
            // Fowo databases copy
            cmds.push_back("mkdir -p /mnt/var/lib/fowo/packages");
            cmds.push_back("cp -a /var/lib/fowo/packages/* /mnt/var/lib/fowo/packages/ 2>/dev/null || true");

            // Execute master install script inside chroot
            cmds.push_back("chroot /mnt /tmp/install_master.sh");

            cmds.push_back("umount /mnt/run");
            cmds.push_back("umount /mnt/sys");
            cmds.push_back("umount /mnt/proc");
            cmds.push_back("umount /mnt/dev");
        }

        // ── Fstab ──
        // Generate fstab using blkid since genfstab is not available on Fedora/Fowo
        cmds.push_back("mkdir -p /mnt/etc");
        cmds.push_back("touch /mnt/etc/fstab");
        for (const auto& disk : ds.disks) {
            for (const auto& part : disk.partitions) {
                if (!part.mount_point.empty()) {
                    std::string dev = part.device;
                    if (dev.find("/dev/") != 0) dev = "/dev/" + dev;
                    if (part.mount_point == "[SWAP]" || part.filesystem == "swap") {
                        cmds.push_back("echo \"UUID=$(blkid -s UUID -o value " + dev + ") none swap defaults 0 0\" >> /mnt/etc/fstab");
                    } else {
                        std::string fs = part.filesystem;
                        if (fs.empty()) fs = "auto";
                        cmds.push_back("echo \"UUID=$(blkid -s UUID -o value " + dev + ") " + part.mount_point + " " + fs + " defaults 0 0\" >> /mnt/etc/fstab");
                    }
                }
            }
        }

        // ── Mount virtual filesystems for the rest of chroot commands ──
        cmds.push_back("mount -o bind /dev /mnt/dev");
        cmds.push_back("mount -t proc proc /mnt/proc");
        cmds.push_back("mount -t sysfs sysfs /mnt/sys");
        cmds.push_back("mount -t tmpfs tmpfs /mnt/run");

        auto chroot_cmd = [](const std::string& cmd) {
            return "chroot /mnt /bin/bash -c '" + cmd + "'";
        };

        // Timezone
        cmds.push_back(chroot_cmd("ln -sf /usr/share/zoneinfo/" + ds.timezone() + " /etc/localtime"));
        if (ds.hwclock_sync)
            cmds.push_back(chroot_cmd("hwclock --systohc || true"));

        // Locale
        for (const auto& loc : ds.selected_locales) {
            cmds.push_back(chroot_cmd("sed -i \"s/#" + loc + "/" + loc + "/\" /etc/locale.gen || true"));
        }
        cmds.push_back(chroot_cmd("locale-gen || true"));
        if (!ds.selected_locales.empty()) {
            cmds.push_back(chroot_cmd("echo LANG=" + ds.selected_locales[0] + " > /etc/locale.conf"));
        }

        // Keyboard
        if (ds.selected_kb_idx < (int)ds.keyboard_layouts.size()) {
            cmds.push_back(chroot_cmd("echo KEYMAP=" + ds.keyboard_layouts[ds.selected_kb_idx].code + " > /etc/vconsole.conf"));
        }

        // Hostname
        cmds.push_back(chroot_cmd("echo " + escape_shell_arg(ds.hostname) + " > /etc/hostname"));
        cmds.push_back(chroot_cmd("echo '127.0.0.1 localhost' >> /etc/hosts"));
        cmds.push_back(chroot_cmd("echo '::1       localhost' >> /etc/hosts"));
        cmds.push_back(chroot_cmd("echo '127.0.1.1 " + escape_shell_arg(ds.hostname) + "' >> /etc/hosts"));

        // Root password
        if (!ds.root_password.empty()) {
            cmds.push_back(chroot_cmd("echo 'root:'" + escape_shell_arg(ds.root_password) + " | chpasswd"));
        }

        // User accounts
        for (const auto& u : ds.users) {
            std::string useradd = "useradd -m";
            if (u.in_wheel) useradd += " -G wheel";
            useradd += " " + escape_shell_arg(u.username);
            cmds.push_back(chroot_cmd(useradd));
            cmds.push_back(chroot_cmd("echo " + escape_shell_arg(u.username + ":" + u.password) + " | chpasswd"));
        }
        // Enable sudo for wheel
        if (!ds.users.empty()) {
            cmds.push_back(chroot_cmd("sed -i 's/# %wheel ALL=(ALL:ALL) ALL/%wheel ALL=(ALL:ALL) ALL/' /etc/sudoers || true"));
        }

        // Bootloader
        if (ds.bootloader == "GRUB") {
            if (ds.is_uefi) {
                cmds.push_back(chroot_cmd("grub2-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=FOWO || grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=FOWO"));
            } else {
                if (!root_device.empty()) {
                    std::string disk_dev = root_device;
                    while (!disk_dev.empty() && isdigit(disk_dev.back())) disk_dev.pop_back();
                    if (!disk_dev.empty() && disk_dev.back() == 'p' && disk_dev.find("nvme") != std::string::npos)
                        disk_dev.pop_back();
                    cmds.push_back(chroot_cmd("grub2-install " + disk_dev + " || grub-install " + disk_dev));
                }
            }
            cmds.push_back(chroot_cmd("grub2-mkconfig -o /boot/grub2/grub.cfg || grub-mkconfig -o /boot/grub/grub.cfg"));
        }

        // Enable services
        for (const auto& svc : ds.enabled_services) {
            cmds.push_back(chroot_cmd("systemctl enable " + svc + " || true"));
        }

        // ZRAM Configuration
        if (ds.zram_enabled) {
            cmds.push_back(chroot_cmd("mkdir -p /etc/systemd"));
            cmds.push_back(chroot_cmd("echo -e '[zram0]\\nzram-size = ram / 2' > /etc/systemd/zram-generator.conf"));
        }

        // NTP
        if (ds.ntp_enabled) {
            cmds.push_back(chroot_cmd("systemctl enable systemd-timesyncd || true"));
        }

        // Unmount
        cmds.push_back("umount -R /mnt/run /mnt/sys /mnt/proc /mnt/dev || true");
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

        ss << "Mode:        " << ds.fowo_install_mode << "\n";
        if (ds.fowo_install_mode == "Master") {
            ss << "Init System: " << ds.fowo_init_choice << "\n";
            ss << "Core Utils:  " << ds.fowo_core_choice << "\n";
        }
        ss << "Hostname:    " << ds.hostname << "\n";
        ss << "Timezone:    " << ds.timezone() << "\n";
        ss << "Bootloader:  " << ds.bootloader << "\n";
        ss << "Boot Mode:   " << (ds.is_uefi ? "UEFI" : "BIOS/Legacy") << "\n";
        
        ss << "Root Pass:   " << (ds.root_password.empty() ? "NOT SET" : "Set") << "\n";
        ss << "Users:       " << ds.users.size() << "\n";

        ss << "Extra Pkgs:  " << ds.additional_packages.size() << "\n";

        return ss.str();
    }
};
