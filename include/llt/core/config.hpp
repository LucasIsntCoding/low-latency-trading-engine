#pragma once

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace llt {

class Config {
public:
    void load_file(const std::string& path) {
        std::ifstream in(path);
        if (!in) {
            throw std::runtime_error("Could not open config file: " + path);
        }
        std::string line;
        while (std::getline(in, line)) {
            auto comment = line.find('#');
            if (comment != std::string::npos) line.erase(comment);
            trim(line);
            if (line.empty()) continue;
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            auto key = line.substr(0, eq);
            auto val = line.substr(eq + 1);
            trim(key); trim(val);
            kv_[key] = val;
        }
    }

    [[nodiscard]] std::string get_string(const std::string& key, const std::string& fallback) const {
        auto it = kv_.find(key);
        return it == kv_.end() ? fallback : it->second;
    }

    [[nodiscard]] std::int64_t get_i64(const std::string& key, std::int64_t fallback) const {
        auto it = kv_.find(key);
        return it == kv_.end() ? fallback : std::stoll(it->second);
    }

    [[nodiscard]] double get_double(const std::string& key, double fallback) const {
        auto it = kv_.find(key);
        return it == kv_.end() ? fallback : std::stod(it->second);
    }

    [[nodiscard]] bool get_bool(const std::string& key, bool fallback) const {
        auto it = kv_.find(key);
        if (it == kv_.end()) return fallback;
        auto v = it->second;
        std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c){ return std::tolower(c); });
        return v == "1" || v == "true" || v == "yes" || v == "on";
    }

private:
    static void trim(std::string& s) {
        auto not_space = [](unsigned char c){ return !std::isspace(c); };
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
        s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
    }

    std::unordered_map<std::string, std::string> kv_;
};

} // namespace llt
