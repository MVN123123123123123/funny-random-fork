#pragma once
// profile_page.hpp - System Profile selection (Desktop/Server/Minimalist)
#include "../../configurations/datastore.hpp"
#include "../../ncurseslib.hpp"
#include "page.hpp"
#include <string>
#include <vector>

class ProfilePage : public Page {
  int selected_ = 0;

public:
  ProfilePage() {}

  std::string title() const override { return "System Profile"; }

  void render(WINDOW *win) override {
    int h, w;
    getmaxyx(win, h, w);
    auto &ds = DataStore::instance();

    wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
    mvwprintw(win, 1, 2, "System Profile");
    wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

    mvwprintw(win, 2, 2, "Select the type of system to install:");

    NcursesLib::draw_hline(win, 3, 1, w - 2);

    for (int i = 0; i < (int)ds.profiles.size() && i < h - 8; i++) {
      auto &p = ds.profiles[i];
      int y = 5 + i * 4; // Spacing for description

      if (i == selected_) {
        wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
        mvwhline(win, y, 2, ' ', w - 4);
        mvwprintw(win, y, 3, "%s  %s",
                  i == ds.selected_profile_idx ? "(*)" : "( )", p.name.c_str());
        wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
      } else {
        int cp = i == ds.selected_profile_idx ? CP_CHECKBOX_ON : CP_NORMAL;
        wattron(win, COLOR_PAIR(cp));
        mvwprintw(win, y, 3, "%s",
                  i == ds.selected_profile_idx ? "(*)" : "( )");
        wattroff(win, COLOR_PAIR(cp));
        wattron(win, COLOR_PAIR(CP_NORMAL));
        mvwprintw(win, y, 7, "%s", p.name.c_str());
        wattroff(win, COLOR_PAIR(CP_NORMAL));
      }

      // Description below
      wattron(win, COLOR_PAIR(CP_SEPARATOR));
      mvwprintw(win, y + 1, 7, "%s", p.description.c_str());
      if (p.name == "Desktop") {
        mvwprintw(win, y + 2, 7, "Selected DE: %s (%s)", ds.selected_de.c_str(),
                  ds.selected_de_flavor.c_str());
      } else if (p.name == "Server") {
        std::string srv_text = "Components: ";
        if (ds.server_components.empty()) {
          srv_text += "(None selected)";
        } else {
          for (size_t ci = 0; ci < ds.server_components.size(); ci++) {
            srv_text += ds.server_components[ci] +
                        (ci < ds.server_components.size() - 1 ? ", " : "");
            if (srv_text.length() > (size_t)w - 20) {
              srv_text += "...";
              break;
            }
          }
        }
        mvwprintw(win, y + 2, 7, "%s", srv_text.c_str());
      } else if (!p.default_de.empty()) {
        mvwprintw(win, y + 2, 7, "Default DE: %s", p.default_de.c_str());
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
    if (ch == KEY_DOWN && selected_ < (int)ds.profiles.size() - 1) {
      selected_++;
      return true;
    }

    if (ch == '\n' || ch == KEY_ENTER) {
      ds.selected_profile_idx = selected_;

      // If Desktop profile selected, configure DE and Flavor
      if (ds.profiles[selected_].name == "Desktop") {
        std::vector<ListOption> de_opts = {
            {"KDE Plasma", "KDE Plasma", ds.selected_de == "KDE Plasma"},
            {"Gnome", "Gnome", ds.selected_de == "Gnome"},
            {"Cinnamon", "Cinnamon", ds.selected_de == "Cinnamon"},
            {"Mate", "Mate", ds.selected_de == "Mate"},
            {"XFCE", "XFCE", ds.selected_de == "XFCE"}};

        if (ListSelectPopup::show(
                "Select Desktop Environment",
                {"Choose your preferred desktop environment."}, de_opts, false,
                false)) {
          for (auto &o : de_opts)
            if (o.selected)
              ds.selected_de = o.value;

          std::vector<ListOption> flavor_opts = {
              {"Bare Minimum", "Bare Minimum (Strictly essentials)",
               ds.selected_de_flavor == "Bare Minimum"},
              {"Usable", "Usable Experience (Recommended)",
               ds.selected_de_flavor == "Usable"},
              {"Full", "Full Experience (Complete set)",
               ds.selected_de_flavor == "Full"}};

          if (ListSelectPopup::show("Select Installation Flavor",
                                    {"Choose how much of " + ds.selected_de +
                                     " you want to install."},
                                    flavor_opts, false, false)) {
            for (auto &o : flavor_opts)
              if (o.selected)
                ds.selected_de_flavor = o.value;
          }
        }
      }
      // If Server profile selected, configure Server components
      else if (ds.profiles[selected_].name == "Server") {
        std::vector<ListOption> srv_opts = {
            {"OpenSSH", "Secure Shell Server (recommended)", false},
            {"Samba", "SMB/CIFS File Sharing for Windows and other Systems",
             false},
            {"Rsync", "Remote File Sync Utility", false},
            {"Nginx", "High-performance Web Server and Proxy", false},
            {"Apache", "Traditional HTTP Server", false},
            {"MariaDB", "SQL Database (MySQL compatible)", false},
            {"PostgreSQL", "Advanced SQL Database", false},
            {"Docker", "Containerization Engine", false},
            {"UFW", "Uncomplicated Firewall", false}};

        // Pre-select existing
        for (auto &o : srv_opts) {
          for (const auto &existing : ds.server_components) {
            if (existing == o.value)
              o.selected = true;
          }
        }

        if (ListSelectPopup::show(
                "Select Server Components",
                {"Choose the services and tools you want to install.",
                 "Press SPACE and ENTER to toggle, ESC to cancel."},
                srv_opts, true, true)) {
          ds.server_components.clear();
          for (const auto &o : srv_opts) {
            if (o.selected)
              ds.server_components.push_back(o.value);
          }
        }
      }

      // ── Fowo Installation Mode ──
      std::vector<ListOption> mode_opts = {
          {"Normal", "Normal (FedOwOra) - Standard Fedora dnf-based with systemd", ds.fowo_install_mode == "Normal"},
          {"Master", "Master (FeOwOra) - Minimal fowo-based OS", ds.fowo_install_mode == "Master"}
      };

      if (ListSelectPopup::show("Fowo Installation Mode",
                                {"Select your installation mode:"},
                                mode_opts, false, false)) {
        for (auto &o : mode_opts)
          if (o.selected)
            ds.fowo_install_mode = o.value;

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
      }
      return true;
    }
    return false;
  }
};
