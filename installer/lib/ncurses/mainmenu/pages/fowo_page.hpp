#pragma once
// fowo_page.hpp - Fowo Installation Mode selection
#include "../../configurations/datastore.hpp"
#include "../../ncurseslib.hpp"
#include "page.hpp"
#include <string>
#include <vector>

class FowoPage : public Page {
  int selected_ = 0;
  std::vector<std::pair<std::string, std::string>> modes_ = {
      {"Normal", "Normal (FedOwOra) - Standard Fedora dnf-based with systemd"},
      {"Master", "Master (FeOwOra) - Minimal fowo-based OS"}
  };

public:
  FowoPage() {}

  std::string title() const override { return "Fowo Installation Mode"; }

  void render(WINDOW *win) override {
    int h, w;
    getmaxyx(win, h, w);
    auto &ds = DataStore::instance();

    wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
    mvwprintw(win, 1, 2, "Fowo Mode");
    wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

    mvwprintw(win, 2, 2, "Select your installation mode:");

    NcursesLib::draw_hline(win, 3, 1, w - 2);

    for (int i = 0; i < (int)modes_.size() && i < h - 8; i++) {
      int y = 5 + i * 4;

      bool is_active = (modes_[i].first == ds.fowo_install_mode);

      if (i == selected_) {
        wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
        mvwhline(win, y, 2, ' ', w - 4);
        mvwprintw(win, y, 3, "%s  %s", is_active ? "(*)" : "( )", modes_[i].first.c_str());
        wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
      } else {
        int cp = is_active ? CP_CHECKBOX_ON : CP_NORMAL;
        wattron(win, COLOR_PAIR(cp));
        mvwprintw(win, y, 3, "%s", is_active ? "(*)" : "( )");
        wattroff(win, COLOR_PAIR(cp));
        wattron(win, COLOR_PAIR(CP_NORMAL));
        mvwprintw(win, y, 7, "%s", modes_[i].first.c_str());
        wattroff(win, COLOR_PAIR(CP_NORMAL));
      }

      wattron(win, COLOR_PAIR(CP_SEPARATOR));
      mvwprintw(win, y + 1, 7, "%s", modes_[i].second.c_str());
      
      if (modes_[i].first == "Master" && ds.fowo_install_mode == "Master") {
          mvwprintw(win, y + 2, 7, "Init: %s | Core: %s", ds.fowo_init_choice.c_str(), ds.fowo_core_choice.c_str());
      }
      wattroff(win, COLOR_PAIR(CP_SEPARATOR));
    }
  }

  bool handle_input(WINDOW *win, int ch) override {
    (void)win;
    auto &ds = DataStore::instance();
    if (ch == KEY_UP && selected_ > 0) {
      selected_--;
      return true;
    }
    if (ch == KEY_DOWN && selected_ < (int)modes_.size() - 1) {
      selected_++;
      return true;
    }

    if (ch == '\n' || ch == KEY_ENTER) {
      ds.fowo_install_mode = modes_[selected_].first;

      if (ds.fowo_install_mode == "Master") {
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
      }
      return true;
    }
    return false;
  }
};
