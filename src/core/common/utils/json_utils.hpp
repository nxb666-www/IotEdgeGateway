#ifndef JSON_UTILS_HPP
#define JSON_UTILS_HPP

#include <string>
#include <sstream>

namespace json_utils {

// JSON 字符串转义：把 " 转成 \"，把 \ 转成 \\.

inline std::string Escape(const std::string& s) {
    std::string result;
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

// 给字符串加引号："hello" → "\"hello\""
inline std::string Quote(const std::string& s) {
    return "\"" + Escape(s) + "\"";
}

// 布尔转字符串
inline std::string Bool(bool v) {
    return v ? "true" : "false";
}

// 数字转字符串
inline std::string Number(int v) {
    return std::to_string(v);
}

inline std::string Number(double v) {
    return std::to_string(v);
}

// 拼 JSON 对象：Object({"key", "value"}, {"key2", "123"})
// 用法：json_utils::Object({{"name", Quote("temp")}, {"value", Number(25)}})
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

} // namespace json_utils

#endif
