#include "language_engine.hpp"
#include <fstream>
#include <iostream>

void LanguageEngine::load_language(const std::string& lang_name) {
    current_language_ = lang_name;
    translations_.clear();
    
    // Try multiple paths depending on where the binary is run from
    std::vector<std::string> search_paths = {
        "/home/linux/funny-random-fork/installer/lib/ncurses/language/translations/",
        "installer/lib/ncurses/language/translations/",
        "../installer/lib/ncurses/language/translations/"
    };
    
    std::ifstream file;
    for (const auto& p : search_paths) {
        std::string filename = p + lang_name + ".lang";
        file.open(filename);
        if (file.is_open()) break;
    }
    
    if (!file.is_open()) return;
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string val = line.substr(pos + 1);
            key.erase(0, key.find_first_not_of(" \t\r\n"));
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            val.erase(0, val.find_first_not_of(" \t\r\n"));
            val.erase(val.find_last_not_of(" \t\r\n") + 1);
            translations_[key] = val;
        }
    }
}

std::string LanguageEngine::translate(const std::string& key) {
    auto it = translations_.find(key);
    if (it != translations_.end()) {
        return it->second;
    }
    return key;
}
