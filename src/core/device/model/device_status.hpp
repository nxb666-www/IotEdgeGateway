#ifndef DEVICE_STATUS_HPP
#define DEVICE_STATUS_HPP

#include <string>
#include <cstdint>

// 设备状态：记录设备的在线状态和最后通信信息
struct DeviceStatus {
    bool online = false;           // 是否在线
    int64_t last_seen_ms = 0;      // 最后一次收到消息的时间戳（毫秒）
    std::string last_payload;      // 最后一次收到的原始数据
    std::string last_topic;        // 最后一次收到消息的 MQTT 主题
};

#endif
