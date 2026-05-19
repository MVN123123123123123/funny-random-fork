#pragma once
// services.hpp - Systemd Services configuration page for HarukaInstaller TUI
#include "page.hpp"
#include "../../ncurseslib.hpp"
#include "../../configurations/datastore.hpp"
#include "../../../variables/packages.hpp"
#include "../../../variables/services_list.hpp"
#include <algorithm>
#include <vector>
#include <string>

class ServicesPage : public Page {
    int selected_idx_ = 0;
    std::vector<ServiceOption> services_;

public:
    ServicesPage() {
        services_ = get_default_services();
    }

    std::string title() const override { return "Systemd Services"; }

    void render(WINDOW* win) override {
        int h, w;
        getmaxyx(win, h, w);
        (void)h;

        auto& ds = DataStore::instance();

        // Title and description
        wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        mvwprintw(win, 1, 2, "Configure System Services (systemd)");
        wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

        mvwprintw(win, 2, 2, "Enable or disable automatic startup services for installed packages.");

        NcursesLib::draw_hline(win, 3, 1, w - 2);

        // ── Render List of Services ─────────────────────────────────────────
        for (size_t i = 0; i < services_.size(); ++i) {
            auto& opt = services_[i];
            bool pkg_installed = is_package_installed(opt.related_package);
            
            // Check if service is enabled in DataStore
            bool is_enabled = false;
            for (const auto& s : ds.enabled_services) {
                if (s == opt.name) {
                    is_enabled = true;
                    break;
                }
            }

            int y = 5 + i * 2;
            bool is_selected = (selected_idx_ == (int)i);

            if (is_selected) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
                mvwhline(win, y, 2, ' ', w - 4);
            }

            if (!pkg_installed) {
                // Dim/Disabled rendering
                if (!is_selected) wattron(win, COLOR_PAIR(CP_SEPARATOR));
                mvwprintw(win, y, 4, "[N/A] %-25s (Package Not Selected)", opt.name.c_str());
                if (!is_selected) wattroff(win, COLOR_PAIR(CP_SEPARATOR));
            } else {
                // Enabled/Toggled rendering
                const char* check = is_enabled ? "[x]" : "[ ]";
                int check_cp = is_enabled ? CP_CHECKBOX_ON : CP_CHECKBOX_OFF;

                if (!is_selected) wattron(win, COLOR_PAIR(check_cp));
                mvwprintw(win, y, 4, "%s", check);
                if (!is_selected) wattroff(win, COLOR_PAIR(check_cp));

                mvwprintw(win, y, 8, "%s", opt.name.c_str());
            }

            if (is_selected) {
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
            }
        }

        NcursesLib::draw_hline(win, 14, 1, w - 2);

        // ── Render Service Details Card ──────────────────────────────────────
        wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        mvwprintw(win, 15, 2, "Service Details:");
        wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

        if (selected_idx_ >= 0 && selected_idx_ < (int)services_.size()) {
            auto& opt = services_[selected_idx_];
            bool pkg_installed = is_package_installed(opt.related_package);
            bool is_enabled = false;
            for (const auto& s : ds.enabled_services) {
                if (s == opt.name) {
                    is_enabled = true;
                    break;
                }
            }

            wattron(win, COLOR_PAIR(CP_NORMAL) | A_BOLD);
            mvwprintw(win, 17, 4, "Name:        %s", opt.name.c_str());
            wattroff(win, COLOR_PAIR(CP_NORMAL) | A_BOLD);

            mvwprintw(win, 18, 4, "Description: %s", opt.description.c_str());
            mvwprintw(win, 19, 4, "Required:    package '%s'", opt.related_package.c_str());

            if (!pkg_installed) {
                wattron(win, COLOR_PAIR(CP_CHECKBOX_OFF) | A_BOLD);
                mvwprintw(win, 21, 4, "Status:      N/A - Related Package Not Installed");
                wattroff(win, COLOR_PAIR(CP_CHECKBOX_OFF) | A_BOLD);
                
                wattron(win, COLOR_PAIR(CP_SEPARATOR));
                mvwprintw(win, 22, 4, "💡 Go to 'System Profile' and choose a profile/flavor");
                mvwprintw(win, 23, 4, "   that installs '%s' to enable this service.", opt.related_package.c_str());
                wattroff(win, COLOR_PAIR(CP_SEPARATOR));
            } else {
                if (is_enabled) {
                    wattron(win, COLOR_PAIR(CP_CHECKBOX_ON) | A_BOLD);
                    mvwprintw(win, 21, 4, "Status:      Enabled (Will start automatically on boot)");
                    wattroff(win, COLOR_PAIR(CP_CHECKBOX_ON) | A_BOLD);
                } else {
                    wattron(win, COLOR_PAIR(CP_ACTION_ITEM) | A_BOLD);
                    mvwprintw(win, 21, 4, "Status:      Disabled (Will not start on boot)");
                    wattroff(win, COLOR_PAIR(CP_ACTION_ITEM) | A_BOLD);
                }
            }
        }

        // Action Hints
        wattron(win, COLOR_PAIR(CP_SEPARATOR));
        mvwprintw(win, h - 2, 2, "Use UP/DOWN to navigate, SPACE or ENTER to toggle active services.");
        wattroff(win, COLOR_PAIR(CP_SEPARATOR));
    }

    bool handle_input(WINDOW* win, int ch) override {
        (void)win;
        auto& ds = DataStore::instance();

        if (ch == KEY_UP) {
            if (selected_idx_ > 0) {
                selected_idx_--;
                return true;
            }
        }
        else if (ch == KEY_DOWN) {
            if (selected_idx_ < (int)services_.size() - 1) {
                selected_idx_++;
                return true;
            }
        }
        else if (ch == ' ' || ch == '\n' || ch == KEY_ENTER) {
            if (selected_idx_ >= 0 && selected_idx_ < (int)services_.size()) {
                auto& opt = services_[selected_idx_];
                if (is_package_installed(opt.related_package)) {
                    // Toggle status in ds.enabled_services
                    auto it = std::find(ds.enabled_services.begin(), ds.enabled_services.end(), opt.name);
                    if (it != ds.enabled_services.end()) {
                        ds.enabled_services.erase(it);
                    } else {
                        ds.enabled_services.push_back(opt.name);
                    }
                    return true;
                }
            }
        }

        return false;
    }
};
