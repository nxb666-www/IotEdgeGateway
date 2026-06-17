#ifndef TIME_UTILS_HPP
#define TIME_UTILS_HPP

#include <string>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>

namespace time_utils {

// 当前 Unix 时间戳（毫秒）
inline int64_t NowUnixMs() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    );
    return ms.count();
}

// 当前时间 ISO 8601 UTC 格式："2026-06-15T10:30:01Z"
inline std::string NowIso8601Utc() {
    std::time_t now = std::time(nullptr);
    std::tm* tm = std::gmtime(&now);
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

// 休眠指定毫秒
inline void SleepMs(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

} // namespace time_utils

#endif
