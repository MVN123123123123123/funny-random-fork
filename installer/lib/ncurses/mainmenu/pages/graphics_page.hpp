#pragma once
// graphics_page.hpp - Graphics Hardware detection and driver selection
#include "../../ncurseslib.hpp"
#include "../../configurations/datastore.hpp"
#include "page.hpp"
#include <string>
#include <vector>

struct GPUInfo {
  std::string name;
  std::string vendor;
  std::string driver_rec;
};

class GraphicsPage : public Page {
  std::vector<GPUInfo> gpus_;
  int selected_ = 0;

  int focus_ = 0; // 0=gpu list

  void init_drivers() {
  }

public:
  GraphicsPage(const std::vector<GPUInfo> &gpus) : gpus_(gpus) {
    init_drivers();
    sync_to_datastore();
  }

  void sync_to_datastore() {
    auto& ds = DataStore::instance();
    ds.gpu_drivers.clear();
    for (size_t i = 0; i < gpus_.size(); i++) {
      GPUDriverSelection sel;
      sel.gpu_name = gpus_[i].name;
      sel.driver_package = "(none)";
      sel.vulkan_package = "(none)";
      ds.gpu_drivers.push_back(sel);
    }
  }

  std::string title() const override { return "Graphics Hardware"; }

  void render(WINDOW *win) override {
    int h, w;
    getmaxyx(win, h, w);

    wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
    mvwprintw(win, 1, 2, "Graphics Hardware Detection");
    wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

    NcursesLib::draw_hline(win, 2, 1, w - 2);

    // GPU list
    wattron(win, COLOR_PAIR(CP_SECTION_TITLE));
    mvwprintw(win, 3, 2, "Detected GPUs:");
    wattroff(win, COLOR_PAIR(CP_SECTION_TITLE));

    for (int i = 0; i < (int)gpus_.size() && i < h - 10; i++) {
      int y = 4 + i;
      if (focus_ == 0 && i == selected_) {
        wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
        mvwhline(win, y, 2, ' ', w - 4);
        mvwprintw(win, y, 3, "[%s] %s", gpus_[i].vendor.c_str(),
                  gpus_[i].name.c_str());
        wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
      } else {
        wattron(win, COLOR_PAIR(i == selected_ ? CP_CHECKBOX_ON : CP_NORMAL));
        mvwprintw(win, y, 3, "%s[%s] %s", i == selected_ ? "> " : "  ",
                  gpus_[i].vendor.c_str(), gpus_[i].name.c_str());
        wattroff(win, COLOR_PAIR(i == selected_ ? CP_CHECKBOX_ON : CP_NORMAL));
      }
    }


  }

  bool handle_input(WINDOW *win, int ch) override {
    (void)win;
    if (focus_ == 0) {
      if (ch == KEY_UP && selected_ > 0) {
        selected_--;
        return true;
      }
      if (ch == KEY_DOWN && selected_ < (int)gpus_.size() - 1) {
        selected_++;
        return true;
      }
    }
    return false;
  }
};
