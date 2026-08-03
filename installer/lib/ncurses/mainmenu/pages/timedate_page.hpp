#pragma once
// timedate_page.hpp - Time and Date Configuration page
// NTP toggle, timezone region/city selection, hwclock sync
#include "page.hpp"
#include "../../ncurseslib.hpp"
#include "../../../variables/regions.hpp"
#include "../../configurations/datastore.hpp"
#include <vector>
#include <string>

class TimeDatePage : public Page {
    std::vector<TZRegion> timezones_;
    int region_idx_ = 0;
    int city_idx_ = 0;
    bool ntp_enabled_ = true;
    bool hwclock_sync_ = true;
    int focus_ = 0; // 0=NTP, 1=hwclock, 2=region, 3=city
    int region_scroll_ = 0;
    int city_scroll_ = 0;

public:
    TimeDatePage(const std::vector<TZRegion>& tz) : timezones_(tz) {
        // Sync initial state from DataStore
        auto& ds = DataStore::instance();
        ntp_enabled_ = ds.ntp_enabled;
        hwclock_sync_ = ds.hwclock_sync;
        for (int i = 0; i < (int)timezones_.size(); i++) {
            if (timezones_[i].region == ds.timezone_region) {
                region_idx_ = i;
                for (int j = 0; j < (int)timezones_[i].cities.size(); j++) {
                    if (timezones_[i].cities[j] == ds.timezone_city) {
                        city_idx_ = j;
                        break;
                    }
                }
                break;
            }
        }
    }

    std::string title() const override { return "Time and Date Configuration"; }

    void render(WINDOW* win) override {
        int h, w;
        getmaxyx(win, h, w);

        wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        mvwprintw(win, 1, 2, "Time and Date Configuration");
        wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

        // Current timezone display
        std::string tz_str = timezones_[region_idx_].region + "/" +
            timezones_[region_idx_].cities[city_idx_];
        mvwprintw(win, 2, 2, "Current Timezone: %s", tz_str.c_str());

        NcursesLib::draw_hline(win, 3, 1, w - 2);

        // ── NTP Toggle ──
        int ntp_cp = focus_ == 0 ? CP_HIGHLIGHT : CP_NORMAL;
        wattron(win, COLOR_PAIR(ntp_cp));
        mvwhline(win, 4, 2, ' ', w - 4);
        mvwprintw(win, 4, 3, "NTP (Network Time Protocol): ");
        wattroff(win, COLOR_PAIR(ntp_cp));
        int ntp_st = ntp_enabled_ ? CP_CHECKBOX_ON : CP_CHECKBOX_OFF;
        wattron(win, COLOR_PAIR(focus_ == 0 ? ntp_cp : ntp_st) | A_BOLD);
        mvwprintw(win, 4, 32, "%s", ntp_enabled_ ? "[ENABLED]" : "[DISABLED]");
        wattroff(win, COLOR_PAIR(focus_ == 0 ? ntp_cp : ntp_st) | A_BOLD);

        // ── hwclock Toggle ──
        int hw_cp = focus_ == 1 ? CP_HIGHLIGHT : CP_NORMAL;
        wattron(win, COLOR_PAIR(hw_cp));
        mvwhline(win, 5, 2, ' ', w - 4);
        mvwprintw(win, 5, 3, "hwclock --systohc:           ");
        wattroff(win, COLOR_PAIR(hw_cp));
        int hw_st = hwclock_sync_ ? CP_CHECKBOX_ON : CP_CHECKBOX_OFF;
        wattron(win, COLOR_PAIR(focus_ == 1 ? hw_cp : hw_st) | A_BOLD);
        mvwprintw(win, 5, 32, "%s", hwclock_sync_ ? "[ENABLED]" : "[DISABLED]");
        wattroff(win, COLOR_PAIR(focus_ == 1 ? hw_cp : hw_st) | A_BOLD);

        NcursesLib::draw_hline(win, 7, 1, w - 2);

        // ── Region / City side by side ──
        int col_w = (w - 6) / 2;
        int list_start = 8;
        int list_h = h - list_start - 2;

        // Region column
        int rcap = focus_ == 2 ? CP_SECTION_TITLE : CP_SEPARATOR;
        wattron(win, COLOR_PAIR(rcap) | A_BOLD);
        mvwprintw(win, list_start, 2, "Region");
        wattroff(win, COLOR_PAIR(rcap) | A_BOLD);

        if (region_idx_ < region_scroll_) region_scroll_ = region_idx_;
        if (region_idx_ >= region_scroll_ + list_h) region_scroll_ = region_idx_ - list_h + 1;

        for (int i = 0; i < list_h && (region_scroll_ + i) < (int)timezones_.size(); i++) {
            int idx = region_scroll_ + i;
            int y = list_start + 1 + i;
            if (focus_ == 2 && idx == region_idx_) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwhline(win, y, 2, ' ', col_w);
                mvwprintw(win, y, 3, "%s", timezones_[idx].region.c_str());
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                wattron(win, COLOR_PAIR(idx == region_idx_ ? CP_CHECKBOX_ON : CP_NORMAL));
                mvwprintw(win, y, 3, "%s%s", idx == region_idx_ ? "> " : "  ",
                    timezones_[idx].region.c_str());
                wattroff(win, COLOR_PAIR(idx == region_idx_ ? CP_CHECKBOX_ON : CP_NORMAL));
            }
        }

        // City column
        int cx = 3 + col_w + 2;
        int ccap = focus_ == 3 ? CP_SECTION_TITLE : CP_SEPARATOR;
        wattron(win, COLOR_PAIR(ccap) | A_BOLD);
        mvwprintw(win, list_start, cx, "City");
        wattroff(win, COLOR_PAIR(ccap) | A_BOLD);

        auto& cities = timezones_[region_idx_].cities;
        if (city_idx_ >= (int)cities.size()) city_idx_ = 0;

        if (city_idx_ < city_scroll_) city_scroll_ = city_idx_;
        if (city_idx_ >= city_scroll_ + list_h) city_scroll_ = city_idx_ - list_h + 1;

        for (int i = 0; i < list_h && (city_scroll_ + i) < (int)cities.size(); i++) {
            int idx = city_scroll_ + i;
            int y = list_start + 1 + i;
            if (focus_ == 3 && idx == city_idx_) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwhline(win, y, cx, ' ', col_w);
                mvwprintw(win, y, cx + 1, "%s", cities[idx].c_str());
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                wattron(win, COLOR_PAIR(idx == city_idx_ ? CP_CHECKBOX_ON : CP_NORMAL));
                mvwprintw(win, y, cx + 1, "%s%s", idx == city_idx_ ? "> " : "  ",
                    cities[idx].c_str());
                wattroff(win, COLOR_PAIR(idx == city_idx_ ? CP_CHECKBOX_ON : CP_NORMAL));
            }
        }
    }

    bool handle_input(WINDOW* win, int ch) override {
        (void)win;
        auto& cities = timezones_[region_idx_].cities;

        // TAB cycles focus through all 4 fields
        if (ch == '\t') { focus_ = (focus_ + 1) % 4; return true; }

        if (focus_ == 0) { // NTP
            if (ch == ' ' || ch == '\n' || ch == KEY_ENTER) {
                ntp_enabled_ = !ntp_enabled_;
                DataStore::instance().ntp_enabled = ntp_enabled_;
                return true;
            }
            if (ch == KEY_DOWN) { focus_ = 1; return true; }
        }
        else if (focus_ == 1) { // hwclock
            if (ch == ' ' || ch == '\n' || ch == KEY_ENTER) {
                hwclock_sync_ = !hwclock_sync_;
                DataStore::instance().hwclock_sync = hwclock_sync_;
                return true;
            }
            if (ch == KEY_UP) { focus_ = 0; return true; }
            if (ch == KEY_DOWN) { focus_ = 2; return true; }
        }
        else if (focus_ == 2) { // Region list
            if (ch == KEY_UP) {
                if (region_idx_ > 0) {
                    region_idx_--;
                    city_idx_ = 0;
                    city_scroll_ = 0;
                    DataStore::instance().timezone_region = timezones_[region_idx_].region;
                    DataStore::instance().timezone_city = timezones_[region_idx_].cities[0];
                } else {
                    focus_ = 1; // Seamlessly go up to hwclock
                }
                return true;
            }
            if (ch == KEY_DOWN) {
                if (region_idx_ < (int)timezones_.size() - 1) {
                    region_idx_++;
                    city_idx_ = 0;
                    city_scroll_ = 0;
                    DataStore::instance().timezone_region = timezones_[region_idx_].region;
                    DataStore::instance().timezone_city = timezones_[region_idx_].cities[0];
                }
                return true;
            }
            if (ch == KEY_RIGHT) {
                focus_ = 3; // Switch side-by-side to City column
                return true;
            }
        }
        else if (focus_ == 3) { // City list
            if (ch == KEY_UP) {
                if (city_idx_ > 0) {
                    city_idx_--;
                    DataStore::instance().timezone_city = cities[city_idx_];
                } else {
                    focus_ = 1; // Seamlessly go up to hwclock
                }
                return true;
            }
            if (ch == KEY_DOWN) {
                if (city_idx_ < (int)cities.size() - 1) {
                    city_idx_++;
                    DataStore::instance().timezone_city = cities[city_idx_];
                }
                return true;
            }
            if (ch == KEY_LEFT) {
                focus_ = 2; // Switch side-by-side to Region column
                return true;
            }
        }
        return false;
    }

};
