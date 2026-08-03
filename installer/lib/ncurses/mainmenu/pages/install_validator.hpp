#pragma once
// install_validator.hpp - Pre-install validation checks
#include "../../configurations/datastore.hpp"
#include <string>
#include <vector>

struct ValidationResult {
    bool pass;
    std::string message;
    bool is_warning; // false = error (blocks install), true = warning (allows proceed)
};

class InstallValidator {
public:
    static std::vector<ValidationResult> validate() {
        std::vector<ValidationResult> results;
        auto& ds = DataStore::instance();

        // Root mountpoint check
        if (!ds.has_root_mountpoint()) {
            results.push_back({false, "No root (/) mountpoint configured on any partition.", false});
        }

        // Root password check
        if (ds.root_password.empty()) {
            results.push_back({false, "Root password is not set.", false});
        }

        // Kernel check
        {
            int count = 0;
            for (const auto& k : ds.kernels) if (k.selected) count++;
            if (count == 0) {
                results.push_back({false, "No kernel selected.", false});
            }
        }

        // Locale check
        if (ds.selected_locales.empty()) {
            results.push_back({false, "No locale selected.", false});
        }

        // EFI partition check for UEFI systems
        if (ds.is_uefi && !ds.has_efi_partition()) {
            results.push_back({false, "UEFI mode detected but no EFI partition (/boot/efi) found.", false});
        }

        // systemd-boot on BIOS
        if (!ds.is_uefi && ds.bootloader == "systemd-boot") {
            results.push_back({false, "systemd-boot requires UEFI mode, but system is in BIOS/Legacy mode.", false});
        }

        // No bootloader warning
        if (ds.bootloader == "None") {
            results.push_back({true, "No bootloader selected. System may not boot without manual setup.", true});
        }

        // No users warning
        if (ds.users.empty()) {
            results.push_back({true, "No user accounts created. Only root will be available.", true});
        }

        // Disk count check
        if (ds.disks.empty()) {
            results.push_back({false, "No storage devices detected.", false});
        }

        return results;
    }

    static bool has_errors(const std::vector<ValidationResult>& results) {
        for (const auto& r : results) {
            if (!r.pass && !r.is_warning) return true;
        }
        return false;
    }
};
