#pragma once
// config_defaults.hpp - Static default configuration lists for HarukaInstaller TUI

#include <string>
#include <vector>

namespace ConfigDefaults {

struct Language {
    std::string code;
    std::string name;
};

inline std::vector<Language> get_languages() {
    return {
        {"en", "English"},
        {"ja", "日本語 (Japanese)"},
        {"de", "Deutsch (German)"},
        {"fr", "Français (French)"},
        {"es", "Español (Spanish)"},
        {"pt", "Português (Portuguese)"},
        {"it", "Italiano (Italian)"},
        {"ko", "한국어 (Korean)"},
        {"zh", "中文 (Chinese)"},
        {"ru", "Русский (Russian)"},
        {"ar", "العربية (Arabic)"},
        {"pl", "Polski (Polish)"},
        {"nl", "Nederlands (Dutch)"},
        {"sv", "Svenska (Swedish)"},
        {"fi", "Suomi (Finnish)"},
        {"tl", "Tagalog (Filipino)"},
    };
}

struct KeyboardLayout {
    std::string code;
    std::string name;
};

inline std::vector<KeyboardLayout> get_keyboard_layouts() {
    return {
        {"us",      "English (US)"},
        {"gb",      "English (UK)"},
        {"de",      "German"},
        {"fr",      "French"},
        {"es",      "Spanish"},
        {"it",      "Italian"},
        {"jp",      "Japanese"},
        {"kr",      "Korean"},
        {"br",      "Portuguese (Brazil)"},
        {"ru",      "Russian"},
        {"dvorak",  "Dvorak"},
        {"colemak", "Colemak"},
    };
}

inline std::vector<std::string> get_locales() {
    return {
        "en_US.UTF-8",
        "en_GB.UTF-8",
        "ja_JP.UTF-8",
        "de_DE.UTF-8",
        "fr_FR.UTF-8",
        "es_ES.UTF-8",
        "it_IT.UTF-8",
        "ko_KR.UTF-8",
        "zh_CN.UTF-8",
        "zh_TW.UTF-8",
        "pt_BR.UTF-8",
        "ru_RU.UTF-8",
        "pl_PL.UTF-8",
        "nl_NL.UTF-8",
        "sv_SE.UTF-8",
        "fi_FI.UTF-8",
    };
}

struct Kernel {
    std::string package;
    std::string description;
};

inline std::vector<Kernel> get_kernels() {
    return {
        {"kernel", "Default FeOwOra Linux kernel"},
    };
}

struct Mirror {
    std::string url;
    std::string country;
    bool        enabled;
};

inline std::vector<Mirror> get_mirrors() {
    return {
        {"https://mirror.fowo.org/repo/$repo/os/$arch",            "Worldwide",   true},
        {"https://us.mirror.fowo.org/repo/$repo/os/$arch",         "United States", true},
        {"https://jp.mirror.fowo.org/repo/$repo/os/$arch",         "Japan",       false},
        {"https://eu.mirror.fowo.org/repo/$repo/os/$arch",         "Germany",     true},
    };
}

struct Profile {
    std::string name;
    std::string description;
    std::string default_de;
};

inline std::vector<Profile> get_profiles() {
    return {
        {"Desktop",    "Full desktop environment with GUI applications", "KDE Plasma"},
        {"Server",     "Headless server with essential tools only",      ""},
        {"Minimalist", "Bare minimum base system, manually configure",   ""},
    };
}

} // namespace ConfigDefaults
