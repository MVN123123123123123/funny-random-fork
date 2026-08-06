#pragma once
// mode_page.hpp - Installation Mode selection (Normal / Master)
#include "page.hpp"
#include "../../ncurseslib.hpp"
#include "../../configurations/datastore.hpp"
#include "../../popups.hpp"
#include <string>
#include <vector>

class ModePage : public Page {
    int selected_ = 0; // 0 = Normal, 1 = Master, 2 = Init System (if Master), 3 = Core Utils (if Master)

public:
    std::string title() const override { return "Installation Mode"; }

    void render(WINDOW* win) override {
        int h, w;
        getmaxyx(win, h, w);
        (void)h;

        auto& ds = DataStore::instance();

        wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        mvwprintw(win, 1, 2, "Installation Mode Configuration");
        wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

        mvwprintw(win, 2, 2, "Choose your installation mode (Normal vs Master):");

        NcursesLib::draw_hline(win, 3, 1, w - 2);

        // 1) Normal Mode
        bool is_normal = (ds.fowo_install_mode == "Normal");
        if (selected_ == 0) {
            wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
            mvwhline(win, 5, 2, ' ', w - 4);
            mvwprintw(win, 5, 3, "%s Normal (FedOwOra) - Standard Fedora dnf-based with systemd",
                      is_normal ? "(*)" : "( )");
            wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
        } else {
            wattron(win, is_normal ? COLOR_PAIR(CP_CHECKBOX_ON) : COLOR_PAIR(CP_NORMAL));
            mvwprintw(win, 5, 3, "%s", is_normal ? "(*)" : "( )");
            wattroff(win, is_normal ? COLOR_PAIR(CP_CHECKBOX_ON) : COLOR_PAIR(CP_NORMAL));
            wattron(win, COLOR_PAIR(CP_NORMAL));
            mvwprintw(win, 5, 7, "Normal (FedOwOra) - Standard Fedora dnf-based with systemd");
            wattroff(win, COLOR_PAIR(CP_NORMAL));
        }
        wattron(win, COLOR_PAIR(CP_SEPARATOR));
        mvwprintw(win, 6, 7, "Standard binary distribution mode using Fedora package manager (dnf).");
        wattroff(win, COLOR_PAIR(CP_SEPARATOR));

        NcursesLib::draw_hline(win, 8, 1, w - 2);

        // 2) Master Mode
        bool is_master = (ds.fowo_install_mode == "Master");
        if (selected_ == 1) {
            wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
            mvwhline(win, 10, 2, ' ', w - 4);
            mvwprintw(win, 10, 3, "%s Master (FeOwOra) - Minimal fowo-based OS",
                      is_master ? "(*)" : "( )");
            wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
        } else {
            wattron(win, is_master ? COLOR_PAIR(CP_CHECKBOX_ON) : COLOR_PAIR(CP_NORMAL));
            mvwprintw(win, 10, 3, "%s", is_master ? "(*)" : "( )");
            wattroff(win, is_master ? COLOR_PAIR(CP_CHECKBOX_ON) : COLOR_PAIR(CP_NORMAL));
            wattron(win, COLOR_PAIR(CP_NORMAL));
            mvwprintw(win, 10, 7, "Master (FeOwOra) - Minimal fowo-based OS");
            wattroff(win, COLOR_PAIR(CP_NORMAL));
        }
        wattron(win, COLOR_PAIR(CP_SEPARATOR));
        mvwprintw(win, 11, 7, "Custom source-compiled minimal build mode using fowo package manager.");
        wattroff(win, COLOR_PAIR(CP_SEPARATOR));

        if (is_master) {
            NcursesLib::draw_hline(win, 13, 1, w - 2);
            wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
            mvwprintw(win, 14, 2, "Master Mode Settings:");
            wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

            // Init System Selection
            if (selected_ == 2) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwhline(win, 16, 2, ' ', w - 4);
                mvwprintw(win, 16, 3, "Init System:     %s (Press ENTER to change)", ds.fowo_init_choice.c_str());
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                wattron(win, COLOR_PAIR(CP_NORMAL));
                mvwprintw(win, 16, 3, "Init System:     %s", ds.fowo_init_choice.c_str());
                wattroff(win, COLOR_PAIR(CP_NORMAL));
            }

            // Core Utils Selection
            if (selected_ == 3) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwhline(win, 18, 2, ' ', w - 4);
                mvwprintw(win, 18, 3, "Core Utilities:  %s (Press ENTER to change)", ds.fowo_core_choice.c_str());
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                wattron(win, COLOR_PAIR(CP_NORMAL));
                mvwprintw(win, 18, 3, "Core Utilities:  %s", ds.fowo_core_choice.c_str());
                wattroff(win, COLOR_PAIR(CP_NORMAL));
            }
        }

        mvwprintw(win, getmaxy(win) - 2, 2, "Use UP/DOWN to navigate, ENTER/SPACE to select.");
    }

    bool handle_input(WINDOW* win, int ch) override {
        (void)win;
        auto& ds = DataStore::instance();
        int max_sel = (ds.fowo_install_mode == "Master") ? 3 : 1;

        if (ch == KEY_UP && selected_ > 0) {
            selected_--;
            return true;
        }
        if (ch == KEY_DOWN && selected_ < max_sel) {
            selected_++;
            return true;
        }

        if (ch == '\n' || ch == KEY_ENTER || ch == ' ') {
            if (selected_ == 0) {
                ds.fowo_install_mode = "Normal";
                return true;
            } else if (selected_ == 1) {
                ds.fowo_install_mode = "Master";
                return true;
            } else if (selected_ == 2 && ds.fowo_install_mode == "Master") {
                std::vector<ListOption> init_opts = {
                    {"systemd", "systemd (default)", ds.fowo_init_choice == "systemd"},
                    {"openrc", "OpenRC", ds.fowo_init_choice == "openrc"},
                    {"runit", "runit", ds.fowo_init_choice == "runit"},
                    {"diy", "Do It Yourself (DIY)", ds.fowo_init_choice == "diy"}
                };

                if (ListSelectPopup::show("Select Init System",
                                          {"Choose the init system for Master mode."},
                                          init_opts, false, false)) {
                    for (auto &o : init_opts)
                        if (o.selected)
                            ds.fowo_init_choice = o.value;
                }
                return true;
            } else if (selected_ == 3 && ds.fowo_install_mode == "Master") {
                std::vector<ListOption> core_opts = {
                    {"coreutils", "coreutils + bash", ds.fowo_core_choice == "coreutils"},
                    {"busybox", "busybox (minimal)", ds.fowo_core_choice == "busybox"}
                };

                if (ListSelectPopup::show("Select Core Utilities",
                                          {"Choose core utilities for Master mode."},
                                          core_opts, false, false)) {
                    for (auto &o : core_opts)
                        if (o.selected)
                            ds.fowo_core_choice = o.value;
                }
                return true;
            }
        }
        return false;
    }
};
