#ifndef CONFIG_MANAGER_HPP
#define CONFIG_MANAGER_HPP

#include <string>
#include <map>
#include <cstdint>

class ConfigManager {
private:
    std::map<std::string, std::string> data_;

public:
    bool LoadYamlFile(const std::string& path);
    bool Has(const std::string& key);
    bool GetString(const std::string& key, std::string& out);
    bool GetInt64(const std::string& key, int64_t& out);
    bool GetBool(const std::string& key, bool& out);
    std::string GetStringOr(const std::string& key, const std::string& default_val);
};

#endif
