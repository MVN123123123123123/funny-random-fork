// mainmenu.cpp - Main menu layout and event loop implementation
#include "mainmenu.hpp"
#include "../configurations/datastore.hpp"
#include "sessionlock.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>

// Page headers
#include "pages/language_page.hpp"
#include "pages/mirror_page.hpp"
#include "pages/keyboard_locale_page.hpp"
#include "pages/disk_page.hpp"
#include "pages/zram_page.hpp"
#include "pages/graphics_page.hpp"
#include "pages/profile_page.hpp"
#include "pages/mode_page.hpp"
#include "pages/network_page.hpp"
#include "pages/accounts_page.hpp"
#include "pages/kernels_page.hpp"
#include "pages/bootloader_page.hpp"
#include "pages/timedate_page.hpp"
#include "pages/services.hpp"
#include "pages/additional_packages_page.hpp"
#include "pages/install_validator.hpp"
#include "pages/installer_backend.hpp"

// Variables
#include "../../variables/regions.hpp"

MainMenu::MainMenu(const std::vector<GPUInfo>& gpus,
                   const std::vector<NetIfaceInfo>& ifaces,
                   const std::vector<WifiNetwork>& wifi,
                   const std::vector<TZRegion>& tz) {
    add_page("Installer Language",          new LanguagePage());
    add_page("Mirror Configuration",        new MirrorPage());
    add_page("Keyboard and Locale",         new KeyboardLocalePage());
    add_page("Storage Device Configuration", new DiskPage());
    add_page("ZRAM and ZSWAP",              new ZramPage());
    add_page("Graphics Hardware",           new GraphicsPage(gpus));
    add_page("Profile",                     new ProfilePage());
    add_page("Installation Mode",           new ModePage());
    add_page("Network",                     new NetworkPage(ifaces, wifi));
    add_page("Root and User Accounts",      new AccountsPage());
    add_page("Kernels",                     new KernelsPage());
    add_page("Bootloader",                  new BootloaderPage());
    add_page("Time and Date",               new TimeDatePage(tz.empty() ? get_regions() : tz));
    add_page("Services",                    new ServicesPage());
    add_page("Additional Packages",         new AdditionalPackagesPage());

    add_separator();
    add_action("Save Config");
    add_action("INSTALL!");
    add_action("Abort");
}

MainMenu::~MainMenu() {
  for (auto &item : items_) {
    delete item.page;
  }
  destroy_windows();
}

void MainMenu::add_page(const std::string &label, Page *page) {
  items_.push_back({label, page, false});
}

void MainMenu::add_separator() { items_.push_back({"---", nullptr, false}); }

void MainMenu::add_action(const std::string &label) {
  items_.push_back({label, nullptr, true});
}

// ── Window Management ───────────────────────────────────────────────────────

void MainMenu::create_windows() {
  int rows = LINES;
  int cols = COLS;

  sidebar_width_ = std::max(28, cols / 4);

  // Title bar: row 0, full width
  win_title_ = newwin(1, cols, 0, 0);

  // Sidebar: rows 1 to LINES-3, left side
  int sidebar_h = rows - 3;
  win_sidebar_ = newwin(sidebar_h, sidebar_width_, 1, 0);

  // Content panel: rows 1 to LINES-3, right side
  int content_w = cols - sidebar_width_;
  win_content_ = newwin(sidebar_h, content_w, 1, sidebar_width_);

  // Status bar: last 2 rows
  win_status_ = newwin(2, cols, rows - 2, 0);

  // Enable keypad for all windows
  keypad(win_sidebar_, TRUE);
  keypad(win_content_, TRUE);

  // Set backgrounds
  NcursesLib::fill_background(win_title_, CP_TITLE_BAR);
  NcursesLib::fill_background(win_sidebar_, CP_NORMAL);
  NcursesLib::fill_background(win_content_, CP_NORMAL);
  NcursesLib::fill_background(win_status_, CP_STATUS_BAR);
}

void MainMenu::destroy_windows() {
  if (win_title_) {
    delwin(win_title_);
    win_title_ = nullptr;
  }
  if (win_sidebar_) {
    delwin(win_sidebar_);
    win_sidebar_ = nullptr;
  }
  if (win_content_) {
    delwin(win_content_);
    win_content_ = nullptr;
  }
  if (win_status_) {
    delwin(win_status_);
    win_status_ = nullptr;
  }
}

// ── Drawing ─────────────────────────────────────────────────────────────────

void MainMenu::draw_title_bar() {
  werase(win_title_);
  wattron(win_title_, COLOR_PAIR(CP_TITLE_BAR) | A_BOLD);
  mvwprintw(win_title_, 0, 1, "%s", Versioning::full_title().c_str());
  wattroff(win_title_, COLOR_PAIR(CP_TITLE_BAR) | A_BOLD);
  wrefresh(win_title_);
}

void MainMenu::draw_sidebar() {
  werase(win_sidebar_);
  int h = getmaxy(win_sidebar_);

  for (int i = 0; i < (int)items_.size() && i < h; i++) {
    auto &item = items_[i];

    // Separator
    if (item.label == "---") {
      NcursesLib::draw_hline(win_sidebar_, i, 0, sidebar_width_ - 1);
      continue;
    }

    bool is_selected = (i == sidebar_cursor_);
    bool focused_sel = is_selected && !content_focused_;

    if (item.is_action) {
      // Action items (Save/Install/Abort)
      if (focused_sel) {
        wattron(win_sidebar_, COLOR_PAIR(CP_ACTION_HIGHLIGHT) | A_BOLD);
        mvwhline(win_sidebar_, i, 0, ' ', sidebar_width_);
        mvwprintw(win_sidebar_, i, 1, "%s", item.label.c_str());
        wattroff(win_sidebar_, COLOR_PAIR(CP_ACTION_HIGHLIGHT) | A_BOLD);
      } else if (is_selected) {
        // Selected but content has focus — dim highlight
        wattron(win_sidebar_, COLOR_PAIR(CP_ACTION_ITEM) | A_BOLD);
        mvwprintw(win_sidebar_, i, 1, "%s", item.label.c_str());
        wattroff(win_sidebar_, COLOR_PAIR(CP_ACTION_ITEM) | A_BOLD);
      } else {
        wattron(win_sidebar_, COLOR_PAIR(CP_ACTION_ITEM));
        mvwprintw(win_sidebar_, i, 1, "%s", item.label.c_str());
        wattroff(win_sidebar_, COLOR_PAIR(CP_ACTION_ITEM));
      }
    } else {
      // Regular menu items
      if (focused_sel) {
        wattron(win_sidebar_, COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
        mvwhline(win_sidebar_, i, 0, ' ', sidebar_width_);
        mvwprintw(win_sidebar_, i, 1, "%s", item.label.c_str());
        wattroff(win_sidebar_, COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
      } else if (is_selected) {
        wattron(win_sidebar_, COLOR_PAIR(CP_HIGHLIGHT));
        mvwhline(win_sidebar_, i, 0, ' ', sidebar_width_);
        mvwprintw(win_sidebar_, i, 1, "%s", item.label.c_str());
        wattroff(win_sidebar_, COLOR_PAIR(CP_HIGHLIGHT));
      } else {
        wattron(win_sidebar_, COLOR_PAIR(CP_NORMAL));
        mvwprintw(win_sidebar_, i, 1, "%s", item.label.c_str());
        wattroff(win_sidebar_, COLOR_PAIR(CP_NORMAL));
      }
    }
  }

  wrefresh(win_sidebar_);
}

void MainMenu::draw_content() {
  werase(win_content_);

  auto &item = items_[sidebar_cursor_];
  if (item.page) {
    // Draw page title in a bordered content panel
    NcursesLib::draw_titled_box(win_content_, item.page->title());

    // Create a sub-window for the page content (inside the border)
    int ch, cw;
    getmaxyx(win_content_, ch, cw);
    WINDOW *inner = derwin(win_content_, ch - 2, cw - 2, 1, 1);
    NcursesLib::fill_background(inner, CP_NORMAL);

    item.page->render(inner);

    delwin(inner);
  } else if (item.is_action) {
    // Show action confirmation area
    NcursesLib::draw_titled_box(win_content_, item.label);
    int ch = getmaxy(win_content_);
    if (item.label == "INSTALL!") {
      NcursesLib::print_center_attr(win_content_, ch / 2 - 1,
                                    "Press ENTER to begin installation",
                                    COLOR_PAIR(CP_ACTION_ITEM) | A_BOLD);
      NcursesLib::print_center(
          win_content_, ch / 2 + 1,
          "(This is a test UI - no actual installation will occur)");
    } else if (item.label == "Save Config") {
      NcursesLib::print_center_attr(win_content_, ch / 2 - 1,
                                    "Press ENTER to save configuration",
                                    COLOR_PAIR(CP_ACTION_ITEM) | A_BOLD);
    } else if (item.label == "Abort") {
      NcursesLib::print_center_attr(win_content_, ch / 2 - 1,
                                    "Press ENTER to abort and exit",
                                    COLOR_PAIR(CP_CHECKBOX_OFF) | A_BOLD);
    }
  } else {
    // Separator selected (shouldn't happen)
    NcursesLib::print_center(win_content_, getmaxy(win_content_) / 2, "---");
  }

  wrefresh(win_content_);
}

void MainMenu::draw_status_bar() {
  werase(win_status_);
  int w = getmaxx(win_status_);

  wattron(win_status_, COLOR_PAIR(CP_STATUS_BAR));

  if (content_focused_) {
    // Content-focused hints
    mvwprintw(win_status_, 0, 1, "Mode:  [Content Configuration]");
    mvwprintw(
        win_status_, 1, 1,
        "Keys:  TAB (Cycle Sections)  BACKSPACE (Back)  ARROWS (Navigate)");
  } else {
    // Sidebar-focused hints
    mvwprintw(win_status_, 0, 1, "Mode:  [Main Navigation]");
    mvwprintw(win_status_, 1, 1,
              "Keys:  ENTER/RIGHT (Select)  UP/DOWN (Navigate)  q (Quit)");
  }

  // Right-aligned help hints
  const char *help1 = "F1 - Quick Help";
  const char *help2 = "Shift+F1 - Wiki Search";
  mvwprintw(win_status_, 0, w - (int)strlen(help1) - 2, "%s", help1);
  mvwprintw(win_status_, 1, w - (int)strlen(help2) - 2, "%s", help2);

  wattroff(win_status_, COLOR_PAIR(CP_STATUS_BAR));
  wrefresh(win_status_);
}

// ── Navigation ──────────────────────────────────────────────────────────────

void MainMenu::cursor_up() {
  int prev = sidebar_cursor_;
  do {
    if (sidebar_cursor_ > 0)
      sidebar_cursor_--;
    else {
      sidebar_cursor_ = prev;
      return;
    }
  } while (items_[sidebar_cursor_].label == "---");
}

void MainMenu::cursor_down() {
  int prev = sidebar_cursor_;
  do {
    if (sidebar_cursor_ < (int)items_.size() - 1)
      sidebar_cursor_++;
    else {
      sidebar_cursor_ = prev;
      return;
    }
  } while (items_[sidebar_cursor_].label == "---");
}

bool MainMenu::handle_action(const std::string &label) {
  if (label == "Abort") {
    if (YesNoPopup::show("Confirm Abort", "Are you sure you want to exit HarukaInstaller?")) {
      werase(win_content_);
      NcursesLib::draw_titled_box(win_content_, "Abort");
      NcursesLib::print_center_attr(win_content_, getmaxy(win_content_) / 2,
                                    "Exiting HarukaInstaller...",
                                    COLOR_PAIR(CP_CHECKBOX_OFF) | A_BOLD);
      wrefresh(win_content_);
      napms(1000);
      return true;
    }
    return false;
  } else if (label == "Save Config") {
    werase(win_content_);
    NcursesLib::draw_titled_box(win_content_, "Save Config");
    auto &ds = DataStore::instance();
    int y = 3;
    mvwprintw(win_content_, y++, 4, "Hostname:    %s", ds.hostname.c_str());
    mvwprintw(win_content_, y++, 4, "Timezone:    %s", ds.timezone().c_str());
    mvwprintw(win_content_, y++, 4, "Bootloader:  %s", ds.bootloader.c_str());
    mvwprintw(win_content_, y++, 4, "Mirrors:     %zu", ds.mirrors.size());
    mvwprintw(win_content_, y++, 4, "Disks:       %zu", ds.disks.size());
    mvwprintw(win_content_, y++, 4, "ZRAM/ZSWAP:  %s/%s",
              ds.zram_enabled ? "ON" : "OFF", ds.zswap_enabled ? "ON" : "OFF");
    mvwprintw(win_content_, y++, 4, "Audio:       %s", ds.audio_system.c_str());
    mvwprintw(win_content_, y++, 4, "Root Pass:   %s", ds.root_password.empty() ? "NOT SET" : "Set");
    mvwprintw(win_content_, y++, 4, "Users:       %zu", ds.users.size());
    mvwprintw(win_content_, y++, 4, "Mode:        %s", ds.fowo_install_mode.c_str());
    mvwprintw(win_content_, y++, 4, "Extra Pkgs:  %zu", ds.additional_packages.size());
    std::string config_summary = InstallerBackend::generate_summary();
    std::string config_json = InstallerBackend::generate_json();
    
    FILE* f_conf = fopen("/var/log/haruka_install.conf", "w");
    if (f_conf) {
      fprintf(f_conf, "%s", config_summary.c_str());
      fclose(f_conf);
    }
    
    FILE* f_json = fopen("/tmp/haruka_install.json", "w");
    if (f_json) {
      fprintf(f_json, "%s", config_json.c_str());
      fclose(f_json);
    }
    
    NcursesLib::print_center_attr(win_content_, getmaxy(win_content_) - 4,
                                  "Configuration saved to /var/log/haruka_install.conf and /tmp/haruka_install.json",
                                  COLOR_PAIR(CP_CHECKBOX_ON) | A_BOLD);
    wrefresh(win_content_);
    napms(2000);
    return false;
  } else if (label == "INSTALL!") {
    // ── Step 1: Validation ──
    auto results = InstallValidator::validate();
    bool has_errors = InstallValidator::has_errors(results);
    
    if (!results.empty()) {
      // Show validation results
      werase(win_content_);
      NcursesLib::draw_titled_box(win_content_, has_errors ? "Validation Errors" : "Validation Warnings");
      int y = 2;
      for (const auto& r : results) {
        int cp = r.is_warning ? CP_ACTION_ITEM : CP_CHECKBOX_OFF;
        wattron(win_content_, COLOR_PAIR(cp) | A_BOLD);
        mvwprintw(win_content_, y, 4, "%s %s", r.is_warning ? "[WARN]" : "[FAIL]", r.message.c_str());
        wattroff(win_content_, COLOR_PAIR(cp) | A_BOLD);
        y++;
      }
      if (has_errors) {
        NcursesLib::print_center_attr(win_content_, getmaxy(win_content_) - 3,
          "Fix the errors above before installing.",
          COLOR_PAIR(CP_CHECKBOX_OFF) | A_BOLD);
        NcursesLib::print_center(win_content_, getmaxy(win_content_) - 2,
          "Press any key to continue.");
        wrefresh(win_content_);
        wgetch(win_content_);
        return false;
      }
      NcursesLib::print_center(win_content_, getmaxy(win_content_) - 2,
        "Press any key to continue.");
      wrefresh(win_content_);
      wgetch(win_content_);
    }

    // ── Step 2: Summary confirmation ──
    std::string summary = InstallerBackend::generate_summary();
    // Show summary in content area
    werase(win_content_);
    NcursesLib::draw_titled_box(win_content_, "Installation Summary");
    int y = 2;
    std::istringstream ss(summary);
    std::string line;
    while (std::getline(ss, line) && y < getmaxy(win_content_) - 4) {
      mvwprintw(win_content_, y++, 4, "%s", line.c_str());
    }
    wrefresh(win_content_);
    
    if (!YesNoPopup::show("Confirm Installation",
          "This will install Fowo Linux with the above settings.",
          "ALL DATA on target partitions will be OVERWRITTEN!")) {
      return false; // User cancelled
    }

    // ── Step 3: Execute installation ──
    auto commands = InstallerBackend::generate_commands();
    werase(win_content_);
    NcursesLib::draw_titled_box(win_content_, "Installing...");
    
    int total = (int)commands.size();
    int max_log_y = getmaxy(win_content_) - 4;
    std::vector<std::string> log_history;
    
    std::ofstream install_log("/var/log/haruka_installer.log", std::ios::out | std::ios::app);
    if (!install_log.is_open()) {
        install_log.open("/tmp/haruka_installer.log", std::ios::out | std::ios::app);
    }
    if (install_log.is_open()) {
        install_log << "=== Starting HarukaInstaller Installation ===" << std::endl;
    }
    
    for (int i = 0; i < total; i++) {
      // Progress bar
      int bar_w = getmaxx(win_content_) - 8;
      int filled = (int)((float)(i + 1) / total * bar_w);
      wattron(win_content_, COLOR_PAIR(CP_STATUS_BAR));
      mvwhline(win_content_, getmaxy(win_content_) - 2, 4, ' ', bar_w);
      wattron(win_content_, COLOR_PAIR(CP_CHECKBOX_ON));
      mvwhline(win_content_, getmaxy(win_content_) - 2, 4, '=', filled);
      wattroff(win_content_, COLOR_PAIR(CP_CHECKBOX_ON));
      mvwprintw(win_content_, getmaxy(win_content_) - 1, 4, "Step %d/%d", i + 1, total);
      
      int max_w = getmaxx(win_content_) - 8;
      auto update_log_view = [&](const std::string& line_str) {
          if (install_log.is_open()) {
              install_log << line_str << std::endl;
          }
          std::string display_line = line_str;
          if ((int)display_line.size() > max_w) display_line = display_line.substr(0, max_w - 3) + "...";
          log_history.push_back(display_line);
          
          int start_idx = 0;
          int available_lines = max_log_y - 2;
          if ((int)log_history.size() > available_lines) {
              start_idx = log_history.size() - available_lines;
          }
          
          // Clear log area
          for (int sy = 2; sy < max_log_y; sy++) {
              mvwhline(win_content_, sy, 4, ' ', max_w + 4);
          }
          
          for (int sy = 0; sy < (int)log_history.size() - start_idx; sy++) {
              wattron(win_content_, COLOR_PAIR(CP_ACTION_ITEM));
              mvwprintw(win_content_, 2 + sy, 4, "%s", log_history[start_idx + sy].c_str());
              wattroff(win_content_, COLOR_PAIR(CP_ACTION_ITEM));
          }
          wrefresh(win_content_);
      };

      update_log_view("> " + commands[i]);
      
      // Execute (in production) or simulate
      #ifdef TESTUI
        napms(200); // Simulate delay in test mode
      #else
        InstallerBackend::execute_command(commands[i], update_log_view);
      #endif
    }
    
    // Done!
    NcursesLib::print_center_attr(win_content_, getmaxy(win_content_) / 2,
      "Installation Complete!", COLOR_PAIR(CP_CHECKBOX_ON) | A_BOLD);
    NcursesLib::print_center(win_content_, getmaxy(win_content_) / 2 + 2,
      "Press any key to exit.");
    wrefresh(win_content_);
    wgetch(win_content_);
    return true; // Signal completion to exit installer
  }
  return false;
}

void MainMenu::handle_resize() {
  destroy_windows();
  endwin();
  refresh();
  create_windows();
}

void MainMenu::show_help() {
  // Show context-sensitive help based on current page
  auto &item = items_[sidebar_cursor_];
  std::string page_title = (item.page) ? item.label : "";
  HelpPopup::show(page_title);
}

// ── Main Event Loop ─────────────────────────────────────────────────────────

void MainMenu::run() {
  create_windows();

  // Initial draw
  draw_title_bar();
  draw_sidebar();
  draw_content();
  draw_status_bar();

  auto redraw_all_cb = [this]() {
    draw_title_bar();
    draw_sidebar();
    draw_content();
    draw_status_bar();
    touchwin(win_title_);
    touchwin(win_sidebar_);
    touchwin(win_content_);
    touchwin(win_status_);
    wrefresh(win_title_);
    wrefresh(win_sidebar_);
    wrefresh(win_content_);
    wrefresh(win_status_);
  };

  bool running = true;
  while (running) {
    // Get input from the appropriate window
    WINDOW *input_win = content_focused_ ? win_content_ : win_sidebar_;
    int ch = wgetch(input_win);

    if (ch == KEY_RESIZE) {
      handle_resize();
    } else if (!content_focused_) {
      // ── SIDEBAR MODE ────────────────────────────────────────────
      switch (ch) {
      case KEY_UP:
        if (SessionLock::check_and_prompt(items_[sidebar_cursor_].page)) {
          cursor_up();
        }
        break;
      case KEY_DOWN:
        if (SessionLock::check_and_prompt(items_[sidebar_cursor_].page)) {
          cursor_down();
        }
        break;
      case KEY_RIGHT:
      case '\t':
      case '\n':
      case KEY_ENTER:
        // Enter the selected page's content panel
        {
          auto &item = items_[sidebar_cursor_];
          if (item.is_action) {
            if (ch == '\n' || ch == KEY_ENTER) {
              bool exit_req = handle_action(item.label);
              if ((item.label == "Abort" || item.label == "INSTALL!") && exit_req) {
                running = false;
              }
            }
          } else if (item.page) {
            content_focused_ = true;
          }
        }
        break;
      case 'q':
      case 'Q':
        if (SessionLock::check_and_prompt(items_[sidebar_cursor_].page)) {
          running = false;
        }
        break;
      case KEY_F(1):
        show_help();
        break;
      case KEY_F(13):
#ifdef KEY_SF1
      case KEY_SF1:
#endif
        HelpSearch::show(redraw_all_cb);
        break;
      default:
        break;
      }
    } else {
      // ── CONTENT MODE ────────────────────────────────────────────
      // Forward input to the page first
      auto &item = items_[sidebar_cursor_];
      bool consumed = false;
      if (item.page) {
        int ph, pw;
        getmaxyx(win_content_, ph, pw);
        WINDOW *inner = derwin(win_content_, ph - 2, pw - 2, 1, 1);
        NcursesLib::fill_background(inner, CP_NORMAL);
        consumed = item.page->handle_input(inner, ch);
        delwin(inner);
      }

      if (!consumed) {
        if (ch == '\t') {
          content_focused_ = false;
        } else if (ch == KEY_F(1)) {
          show_help();
        } else if (ch == KEY_F(13)
#ifdef KEY_SF1
                   || ch == KEY_SF1
#endif
        ) {
          HelpSearch::show(redraw_all_cb);
        } else if (NcursesLib::is_back_key(ch)) {
          content_focused_ = false;
        }
      }
    }

    // Redraw everything
    draw_title_bar();
    draw_sidebar();
    draw_content();
    draw_status_bar();
  }
}
