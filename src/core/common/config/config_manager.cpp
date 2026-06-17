#include "config_manager.hpp"
#include <ryml.hpp>
#include <ryml_std.hpp>
#include <fstream>
#include <sstream>

using std::string;

bool ConfigManager::LoadYamlFile(const string& path) {
    // 读文件 → rapidyaml 解析 → 存到 data_
    std::ifstream file(path);
    if (!file.is_open()) return false;
    std::stringstream ss;
    ss << file.rdbuf();
    string content = ss.str();

    ryml::Tree tree = ryml::parse(c4::to_csubstr(content));

    return true;
}

bool ConfigManager::Has(const string& key) {
    // data_ 里有没有这个 key
    return data_.count(key);
}

bool ConfigManager::GetString(const string& key, string& out) {
    // 从 data_ 里取值
    if (Has(key)) {
        out = data_[key];
        return true;
    }
    return false;
}

bool ConfigManager::GetInt64(const string& key, int64_t& out) {
    // 取值并转成数字
    string val;
    if (!GetString(key, val)) return false;
    out = std::stoll(val);
    return true;
}

bool ConfigManager::GetBool(const string& key, bool& out) {
    // 取值并转成布尔
    string val;
    if (!GetString(key, val)) return false;
    out = (val=="true");
    return true;
}

string ConfigManager::GetStringOr(const string& key, const string& default_val) {
    // 有就返回值，没有就返回默认值
    string out;
    if (GetString(key, out)) {
        return out;
    }
    return default_val;
}
