#pragma once
// bootloader_page.hpp - Bootloader Selection page
#include "../../ncurseslib.hpp"
#include "../../configurations/datastore.hpp"
#include "page.hpp"
#include <string>
#include <vector>

struct BootloaderOption {
  std::string name;
  std::string description;
};

class BootloaderPage : public Page {
  std::vector<BootloaderOption> options_;
  int selected_ = 0;

public:
  BootloaderPage() {
    options_ = {
        {"GRUB", "The GNU Grand Unified Bootloader. Highly compatible and "
                 "feature-rich."},
        {"systemd-boot",
         "A simple UEFI boot manager. Fast and minimalist. (UEFI only)"},
        {"None", "I will handle this myself. (Expert only)"}};

    // Sync initial selection from DataStore
    auto& ds = DataStore::instance();
    for (int i = 0; i < (int)options_.size(); i++) {
      if (options_[i].name == ds.bootloader) {
        selected_ = i;
        break;
      }
    }
  }

  std::string title() const override { return "Bootloader Selection"; }

  void render(WINDOW *win) override {
    int h, w;
    getmaxyx(win, h, w);
    auto& ds = DataStore::instance();

    wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
    mvwprintw(win, 1, 2, "Select Bootloader");
    wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

    // UEFI/BIOS indicator
    mvwprintw(win, 2, 2, "Boot Mode: %s", ds.is_uefi ? "UEFI" : "BIOS/Legacy");
    if (!ds.is_uefi && ds.bootloader == "systemd-boot") {
      wattron(win, COLOR_PAIR(CP_CHECKBOX_OFF) | A_BOLD);
      mvwprintw(win, 2, 35, "(systemd-boot requires UEFI!)");
      wattroff(win, COLOR_PAIR(CP_CHECKBOX_OFF) | A_BOLD);
    }

    for (int i = 0; i < (int)options_.size(); i++) {
      int y = 4 + i * 2;
      bool is_sel = (i == selected_);
      bool is_active = (options_[i].name == ds.bootloader);

      if (is_sel) {
        wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
        mvwhline(win, y, 2, ' ', w - 4);
        mvwprintw(win, y, 4, "(%c) %s", is_active ? '*' : ' ', options_[i].name.c_str());
        wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
      } else {
        int cp = is_active ? CP_CHECKBOX_ON : CP_NORMAL;
        wattron(win, COLOR_PAIR(cp));
        mvwprintw(win, y, 4, "(%c) %s", is_active ? '*' : ' ', options_[i].name.c_str());
        wattroff(win, COLOR_PAIR(cp));
      }

      // Description
      wattron(win, COLOR_PAIR(CP_SEPARATOR));
      mvwprintw(win, y + 1, 8, "%s", options_[i].description.c_str());
      wattroff(win, COLOR_PAIR(CP_SEPARATOR));
    }

    mvwprintw(win, h - 2, 2, "Use UP/DOWN to navigate, ENTER/SPACE to select.");
  }

  bool handle_input(WINDOW *win, int ch) override {
    (void)win;
    if (ch == KEY_UP && selected_ > 0) {
      selected_--;
      return true;
    }
    if (ch == KEY_DOWN && selected_ < (int)options_.size() - 1) {
      selected_++;
      return true;
    }
    if (ch == '\n' || ch == KEY_ENTER || ch == ' ') {
      DataStore::instance().bootloader = options_[selected_].name;
      return true;
    }
    return false;
  }
};
