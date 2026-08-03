#pragma once
// accounts_page.hpp - Root and User Accounts configuration
#include "page.hpp"
#include "../../ncurseslib.hpp"
#include "../../configurations/datastore.hpp"
#include <vector>
#include <string>

class AccountsPage : public Page {
    int focus_ = 0;       // 0=root section, 1=user list, 2=add user
    int user_selected_ = 0;

public:
    AccountsPage() {}

    std::string title() const override { return "Root and User Accounts"; }

    void render(WINDOW* win) override {
        int h, w;
        getmaxyx(win, h, w);
        auto& ds = DataStore::instance();

        // ── Root Password Section ──
        wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        mvwprintw(win, 1, 2, "Root Account");
        wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

        mvwprintw(win, 3, 4, "Root Password: ");
        if (ds.root_password.empty()) {
            wattron(win, COLOR_PAIR(CP_CHECKBOX_OFF));
            mvwprintw(win, 3, 19, "[NOT SET]");
            wattroff(win, COLOR_PAIR(CP_CHECKBOX_OFF));
        } else {
            wattron(win, COLOR_PAIR(CP_CHECKBOX_ON));
            mvwprintw(win, 3, 19, "[SET - %d chars]", (int)ds.root_password.size());
            wattroff(win, COLOR_PAIR(CP_CHECKBOX_ON));
        }

        if (focus_ == 0) {
            wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
            mvwprintw(win, 4, 4, " [Press ENTER to set root password] ");
            wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
        } else {
            mvwprintw(win, 4, 4, "  Press ENTER to set root password  ");
        }

        NcursesLib::draw_hline(win, 6, 1, w - 2);

        // ── User Accounts Section ──
        wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        mvwprintw(win, 7, 2, "User Accounts");
        wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

        if (ds.users.empty()) {
            mvwprintw(win, 9, 4, "No users created yet.");
        } else {
            wattron(win, COLOR_PAIR(CP_TABLE_HEADER) | A_BOLD | A_UNDERLINE);
            mvwprintw(win, 9, 4, "%-20s %-10s %-8s", "Username", "Password", "Wheel");
            wattroff(win, COLOR_PAIR(CP_TABLE_HEADER) | A_BOLD | A_UNDERLINE);

            for (int i = 0; i < (int)ds.users.size() && i < h - 14; i++) {
                int y = 10 + i;
                if (focus_ == 1 && i == user_selected_) {
                    wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                    mvwhline(win, y, 3, ' ', w - 6);
                } else {
                    wattron(win, COLOR_PAIR(CP_NORMAL));
                }
                mvwprintw(win, y, 4, "%-20s %-10s %-8s",
                    ds.users[i].username.c_str(),
                    "********",
                    ds.users[i].in_wheel ? "Yes" : "No");
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
                wattroff(win, COLOR_PAIR(CP_NORMAL));
            }
        }

        // Add/Delete user actions
        int act_y = std::max(11, 10 + (int)ds.users.size() + 1);
        if (act_y < h - 3) {
            NcursesLib::draw_hline(win, act_y, 1, w - 2);
            if (focus_ == 2) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwprintw(win, act_y + 1, 4, " [Add New User] ");
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                mvwprintw(win, act_y + 1, 4, "  Add New User  ");
            }

            if (!ds.users.empty()) {
                mvwprintw(win, act_y + 2, 4, "  DEL key to remove selected user");
            }
        }
    }

    bool handle_input(WINDOW* win, int ch) override {
        auto& ds = DataStore::instance();

        if (ch == '\t') {
            focus_ = (focus_ + 1) % 3;
            if (focus_ == 1 && ds.users.empty()) focus_ = 2;
            return true;
        }

        if (focus_ == 0) {
            if (ch == '\n' || ch == KEY_ENTER) {
                std::string pw = NcursesLib::masked_input(win, 4, 5, 30, 64);
                if (!pw.empty()) ds.root_password = pw;
                return true;
            }
        } else if (focus_ == 1) {
            if (ch == KEY_UP && user_selected_ > 0) { user_selected_--; return true; }
            if (ch == KEY_DOWN && user_selected_ < (int)ds.users.size() - 1) { user_selected_++; return true; }
            if ((ch == KEY_DC || ch == 'd') && !ds.users.empty()) {
                ds.users.erase(ds.users.begin() + user_selected_);
                if (user_selected_ >= (int)ds.users.size() && user_selected_ > 0)
                    user_selected_--;
                if (ds.users.empty()) focus_ = 2;
                return true;
            }
        } else if (focus_ == 2) {
            if (ch == '\n' || ch == KEY_ENTER) {
                std::string uname = NcursesLib::text_input(win, 7, 18, 20, 32);
                if (!uname.empty()) {
                    std::string pw = NcursesLib::masked_input(win, 8, 18, 20, 64);
                    if (!pw.empty()) {
                        ds.users.push_back({uname, pw, true});
                    }
                }
                return true;
            }
        }
        return false;
    }
};
