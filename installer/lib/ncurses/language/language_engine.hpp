#pragma once
#include <string>
#include <unordered_map>
#include <vector>

class LanguageEngine {
private:
    std::unordered_map<std::string, std::string> translations_;
    std::string current_language_ = "English";

    LanguageEngine() = default;

public:
    static LanguageEngine& instance() {
        static LanguageEngine instance;
        return instance;
    }

    void load_language(const std::string& lang_name);
    std::string translate(const std::string& key);
    
    const std::string& get_current_language() const { return current_language_; }
};

// Global macro for convenience
#define _TR(key) LanguageEngine::instance().translate(key)
