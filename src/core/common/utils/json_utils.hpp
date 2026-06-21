#ifndef JSON_UTILS_HPP
#define JSON_UTILS_HPP

#include <initializer_list>
#include <sstream>
#include <string>
#include <utility>

namespace json_utils {

inline std::string Escape(const std::string& s) {
    std::string result;
    result.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n";  break;
            case '\r': result += "\\r";  break;
            case '\t': result += "\\t";  break;
            default:   result += c;      break;
        }
    }
    return result;
}

inline std::string Quote(const std::string& s) {
    return "\"" + Escape(s) + "\"";
}

inline std::string Bool(bool v) {
    return v ? "true" : "false";
}

inline std::string Number(int v) {
    return std::to_string(v);
}

inline std::string Number(double v) {
    return std::to_string(v);
}

inline std::string Object(std::initializer_list<std::pair<std::string, std::string>> fields) {
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& f : fields) {
        if (!first) oss << ",";
        oss << Quote(f.first) << ":" << f.second;
        first = false;
    }
    oss << "}";
    return oss.str();
}

inline std::string ExtractNumber(const std::string& json, size_t key_pos) {
    size_t colon = json.find(':', key_pos);
    if (colon == std::string::npos) return "0";

    size_t start = colon + 1;
    while (start < json.length() && json[start] == ' ') start++;

    size_t end = start;
    if (end < json.length() && json[end] == '-') end++;
    while (end < json.length() &&
           ((json[end] >= '0' && json[end] <= '9') || json[end] == '.')) {
        end++;
    }

    if (end == start) return "0";
    return json.substr(start, end - start);
}

inline std::string ExtractValue(const std::string& json) {
    size_t data_pos = json.find("\"data\"");
    if (data_pos != std::string::npos) {
        size_t val_pos = json.find("\"value\"", data_pos);
        if (val_pos != std::string::npos) {
            return ExtractNumber(json, val_pos);
        }
    }

    size_t val_pos = json.find("\"value\"");
    if (val_pos != std::string::npos) {
        return ExtractNumber(json, val_pos);
    }

    if (json.empty()) return "\"\"";
    return json;
}

inline int GetJsonInt(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return 0;

    pos = json.find(':', pos + search.length());
    if (pos == std::string::npos) return 0;
    pos++;

    while (pos < json.length() && json[pos] == ' ') pos++;

    bool negative = false;
    if (pos < json.length() && json[pos] == '-') {
        negative = true;
        pos++;
    }

    int value = 0;
    while (pos < json.length() && json[pos] >= '0' && json[pos] <= '9') {
        value = value * 10 + (json[pos] - '0');
        pos++;
    }

    return negative ? -value : value;
}

} // namespace json_utils

#endif
