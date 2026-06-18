#include "config_manager.hpp"
#include <ryml.hpp>
#include <ryml_std.hpp>
#include <fstream>
#include <sstream>

using std::string;

// 递归遍历 YAML 节点，把嵌套的 key 用 "." 连起来
// 比如 network → http_api → host 存成 "network.http_api.host"
static void FlattenNode(ryml::NodeRef node, const string& prefix, std::map<string, string>& out) {
    if (node.has_children()) {
        // 有子节点 → 是 map 或 list，继续递归
        for (size_t i = 0; i < node.num_children(); i++) {
            auto child = node.child(i);
            string key;
            if (child.key().len > 0) {
                key = prefix.empty()
                    ? string(child.key().str, child.key().len)
                    : prefix + "." + string(child.key().str, child.key().len);
            } else {
                // list 元素没有 key，用下标
                key = prefix + "[" + std::to_string(i) + "]";
            }
            FlattenNode(child, key, out);
        }
    } else if (node.has_val()) {
        // 叶子节点 → 存值
        out[prefix] = string(node.val().str, node.val().len);
    }
}

bool ConfigManager::LoadYamlFile(const string& path) {
    // 读文件 → rapidyaml 解析 → 存到 data_
    std::ifstream file(path);
    if (!file.is_open()) return false;
    std::stringstream ss;
    ss << file.rdbuf();
    string content = ss.str();

    ryml::Tree tree = ryml::parse_in_arena(c4::to_csubstr(content));
    FlattenNode(tree.rootref(), "", data_);

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
