#pragma once
// audio.hpp - Audio Configuration page for HarukaInstaller TUI
#include "page.hpp"
#include "../../ncurseslib.hpp"
#include "../../configurations/datastore.hpp"

class AudioPage : public Page {
    int selected_idx_ = 0; // 0=PipeWire, 1=PulseAudio, 2=None
    bool first_load_ = true;

public:
    std::string title() const override { return "Audio Configuration"; }

    void render(WINDOW* win) override {
        int h, w;
        getmaxyx(win, h, w);
        (void)h;

        auto& ds = DataStore::instance();

        // Sync initial selection index with DataStore value only on first load
        if (first_load_) {
            if (ds.audio_system == "PipeWire") selected_idx_ = 0;
            else if (ds.audio_system == "PulseAudio") selected_idx_ = 1;
            else selected_idx_ = 2;
            first_load_ = false;
        }

        // Title and description
        wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        mvwprintw(win, 1, 2, "Select Multimedia / Sound Server Framework");
        wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

        mvwprintw(win, 2, 2, "Choose the audio system to handle sound playback and device routing.");

        NcursesLib::draw_hline(win, 3, 1, w - 2);

        // ── Render Choices ──────────────────────────────────────────────────
        std::vector<std::pair<std::string, std::string>> options = {
            {"PipeWire (Recommended)", "PipeWire"},
            {"PulseAudio (Legacy)",    "PulseAudio"},
            {"No Audio Server (Headless/None)", "None"}
        };

        for (int i = 0; i < 3; ++i) {
            bool is_selected = (selected_idx_ == i);
            bool is_active = (ds.audio_system == options[i].second);

            if (is_selected) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
                mvwhline(win, 5 + i * 2, 2, ' ', w - 4);
            }

            // Radio button mark
            const char* radio = is_active ? "(*) " : "( ) ";
            int radio_cp = is_active ? CP_CHECKBOX_ON : CP_CHECKBOX_OFF;

            if (!is_selected) wattron(win, COLOR_PAIR(radio_cp));
            mvwprintw(win, 5 + i * 2, 4, "%s", radio);
            if (!is_selected) wattroff(win, COLOR_PAIR(radio_cp));

            mvwprintw(win, 5 + i * 2, 8, "%s", options[i].first.c_str());

            if (is_selected) {
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
            }
        }

        NcursesLib::draw_hline(win, 11, 1, w - 2);

        // ── Render Information / Card Panel ──────────────────────────────────
        wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        mvwprintw(win, 12, 2, "System Details & Recommendation:");
        wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

        if (selected_idx_ == 0) {
            // PipeWire Info
            wattron(win, COLOR_PAIR(CP_CHECKBOX_ON) | A_BOLD);
            mvwprintw(win, 14, 4, "[RECOMMENDED] PipeWire Multimedia Server");
            wattroff(win, COLOR_PAIR(CP_CHECKBOX_ON) | A_BOLD);

            wattron(win, COLOR_PAIR(CP_NORMAL));
            mvwprintw(win, 15, 4, "PipeWire is the modern standard for Linux audio and video.");
            mvwprintw(win, 16, 4, "• Native low-latency real-time performance.");
            mvwprintw(win, 17, 4, "• Out-of-the-box compatibility with PulseAudio, JACK, and ALSA clients.");
            mvwprintw(win, 18, 4, "• Robust security model and sandboxing support (Flatpak friendly).");
            wattroff(win, COLOR_PAIR(CP_NORMAL));
        }
        else if (selected_idx_ == 1) {
            // PulseAudio Info & Warnings
            wattron(win, COLOR_PAIR(CP_CHECKBOX_OFF) | A_BOLD);
            mvwprintw(win, 14, 4, "[⚠️ LEGACY WARNING] PulseAudio Sound Server");
            wattroff(win, COLOR_PAIR(CP_CHECKBOX_OFF) | A_BOLD);

            wattron(win, COLOR_PAIR(CP_NORMAL));
            mvwprintw(win, 15, 4, "PulseAudio is the legacy Linux sound server framework.");
            wattron(win, COLOR_PAIR(CP_CHECKBOX_OFF));
            mvwprintw(win, 16, 4, "• WARNING: Deprecated in major distributions.");
            mvwprintw(win, 17, 4, "• Lacks modern optimizations, low-latency JACK integrations, and pro-audio modes.");
            mvwprintw(win, 18, 4, "• Only select this if you have specific hardware or software requiring legacy stack.");
            wattroff(win, COLOR_PAIR(CP_CHECKBOX_OFF));
        }
        else {
            // None Info
            wattron(win, COLOR_PAIR(CP_ACTION_ITEM) | A_BOLD);
            mvwprintw(win, 14, 4, "[MINIMALIST] No Audio Server");
            wattroff(win, COLOR_PAIR(CP_ACTION_ITEM) | A_BOLD);

            wattron(win, COLOR_PAIR(CP_NORMAL));
            mvwprintw(win, 15, 4, "Disables installation of default audio servers and configurations.");
            mvwprintw(win, 16, 4, "• Prevents background audio daemons from running.");
            mvwprintw(win, 17, 4, "• Highly recommended for headless servers or containers.");
            mvwprintw(win, 18, 4, "• You can manually install ALSA or custom drivers later.");
            wattroff(win, COLOR_PAIR(CP_NORMAL));
        }

        // Action Hints
        wattron(win, COLOR_PAIR(CP_SEPARATOR));
        mvwprintw(win, h - 2, 2, "Use UP/DOWN to navigate, SPACE or ENTER to select the active server.");
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
            if (selected_idx_ < 2) {
                selected_idx_++;
                return true;
            }
        }
        else if (ch == ' ' || ch == '\n' || ch == KEY_ENTER) {
            if (selected_idx_ == 0) ds.audio_system = "PipeWire";
            else if (selected_idx_ == 1) ds.audio_system = "PulseAudio";
            else ds.audio_system = "None";
            return true;
        }

        return false;
    }
};
