#pragma once
// additional_packages_page.hpp - User-specified additional packages
#include "page.hpp"
#include "../../ncurseslib.hpp"
#include "../../configurations/datastore.hpp"
#include "../../popups.hpp"
#include "../../error_popup.hpp"
#include <vector>
#include <string>

class AdditionalPackagesPage : public Page {
    int focus_ = 0; // 0=package list, 1=add, 2=remove, 3=clear
    int selected_ = 0;
    int scroll_ = 0;

public:
    AdditionalPackagesPage() {}

    std::string title() const override { return "Additional Packages"; }

    void render(WINDOW* win) override {
        int h, w;
        getmaxyx(win, h, w);
        auto& ds = DataStore::instance();

        wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        mvwprintw(win, 1, 2, "Additional Packages");
        wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

        mvwprintw(win, 2, 2, "Add extra packages to install alongside the base system.");

        NcursesLib::draw_hline(win, 3, 1, w - 2);

        // ── Package list ──
        int list_h = h - 12;
        if (list_h < 3) list_h = 3;

        if (ds.additional_packages.empty()) {
            wattron(win, COLOR_PAIR(CP_SEPARATOR));
            mvwprintw(win, 5, 4, "No additional packages added yet.");
            mvwprintw(win, 6, 4, "Use 'Add Package' below to add packages.");
            wattroff(win, COLOR_PAIR(CP_SEPARATOR));
        } else {
            wattron(win, COLOR_PAIR(CP_TABLE_HEADER) | A_BOLD | A_UNDERLINE);
            mvwprintw(win, 4, 4, "%-40s %s", "Package Name", "Status");
            wattroff(win, COLOR_PAIR(CP_TABLE_HEADER) | A_BOLD | A_UNDERLINE);

            if (selected_ < scroll_) scroll_ = selected_;
            if (selected_ >= scroll_ + list_h) scroll_ = selected_ - list_h + 1;

            for (int i = 0; i < list_h && (scroll_ + i) < (int)ds.additional_packages.size(); i++) {
                int idx = scroll_ + i;
                int y = 5 + i;
                if (focus_ == 0 && idx == selected_) {
                    wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                    mvwhline(win, y, 3, ' ', w - 6);
                    mvwprintw(win, y, 4, "> %-40s Queued", ds.additional_packages[idx].c_str());
                    wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
                } else {
                    wattron(win, COLOR_PAIR(CP_NORMAL));
                    mvwprintw(win, y, 4, "  %-40s Queued", ds.additional_packages[idx].c_str());
                    wattroff(win, COLOR_PAIR(CP_NORMAL));
                }
            }
        }

        // ── Actions ──
        int act_y = h - 6;
        NcursesLib::draw_hline(win, act_y - 1, 1, w - 2);

        const char* actions[] = {"Add Package(s)", "Remove Selected", "Clear All"};
        for (int i = 0; i < 3; i++) {
            int y = act_y + i;
            bool is_sel = (focus_ == i + 1);
            if (is_sel) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwhline(win, y, 3, ' ', w - 6);
                mvwprintw(win, y, 4, "> %s", actions[i]);
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                mvwprintw(win, y, 4, "  %s", actions[i]);
            }
        }

        // Summary
        wattron(win, COLOR_PAIR(CP_STATUS_BAR));
        mvwhline(win, h - 1, 0, ' ', w);
        mvwprintw(win, h - 1, 2, "Total packages: %zu | TAB to switch sections | ENTER to act",
                  ds.additional_packages.size());
        wattroff(win, COLOR_PAIR(CP_STATUS_BAR));
    }

    bool handle_input(WINDOW* win, int ch) override {
        auto& ds = DataStore::instance();

        if (ch == '\t') {
            focus_ = (focus_ + 1) % 4;
            if (focus_ == 0 && ds.additional_packages.empty()) focus_ = 1;
            return true;
        }

        if (focus_ == 0) {
            // Package list navigation
            if (ds.additional_packages.empty()) { focus_ = 1; return true; }
            if (ch == KEY_UP && selected_ > 0) { selected_--; return true; }
            if (ch == KEY_DOWN && selected_ < (int)ds.additional_packages.size() - 1) { selected_++; return true; }
            if ((ch == KEY_DC || ch == 'd' || ch == 'D') && !ds.additional_packages.empty()) {
                ds.additional_packages.erase(ds.additional_packages.begin() + selected_);
                if (selected_ >= (int)ds.additional_packages.size() && selected_ > 0) selected_--;
                if (ds.additional_packages.empty()) focus_ = 1;
                return true;
            }
            return false;
        }

        if (focus_ == 1) {
            // Add Package
            if (ch == KEY_UP) { focus_ = 0; return true; }
            if (ch == KEY_DOWN) { focus_ = 2; return true; }
            if (ch == '\n' || ch == KEY_ENTER) {
                std::string input = InputPopup::show("Add Packages",
                    "Enter package name(s), space-separated:", "");
                if (!input.empty()) {
                    // Split by spaces
                    std::string pkg;
                    for (size_t i = 0; i <= input.size(); i++) {
                        if (i == input.size() || input[i] == ' ') {
                            if (!pkg.empty()) {
                                // Check for duplicates
                                bool exists = false;
                                for (const auto& p : ds.additional_packages) {
                                    if (p == pkg) { exists = true; break; }
                                }
                                if (!exists) {
                                    ds.additional_packages.push_back(pkg);
                                }
                                pkg.clear();
                            }
                        } else {
                            pkg += input[i];
                        }
                    }
                }
                return true;
            }
        }

        if (focus_ == 2) {
            // Remove Selected
            if (ch == KEY_UP) { focus_ = 1; return true; }
            if (ch == KEY_DOWN) { focus_ = 3; return true; }
            if (ch == '\n' || ch == KEY_ENTER) {
                if (!ds.additional_packages.empty() && selected_ < (int)ds.additional_packages.size()) {
                    if (YesNoPopup::show("Remove Package",
                            "Remove '" + ds.additional_packages[selected_] + "'?")) {
                        ds.additional_packages.erase(ds.additional_packages.begin() + selected_);
                        if (selected_ >= (int)ds.additional_packages.size() && selected_ > 0) selected_--;
                    }
                } else {
                    ErrorPopup::show("Notice", "No packages to remove.");
                }
                return true;
            }
        }

        if (focus_ == 3) {
            // Clear All
            if (ch == KEY_UP) { focus_ = 2; return true; }
            if (ch == '\n' || ch == KEY_ENTER) {
                if (!ds.additional_packages.empty()) {
                    if (YesNoPopup::show("Clear All", "Remove all additional packages?")) {
                        ds.additional_packages.clear();
                        selected_ = 0;
                    }
                } else {
                    ErrorPopup::show("Notice", "List is already empty.");
                }
                return true;
            }
        }

        return false;
    }
};
