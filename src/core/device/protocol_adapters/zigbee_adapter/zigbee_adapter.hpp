#ifndef ZIGBEE_ADAPTER_HPP
#define ZIGBEE_ADAPTER_HPP

#include <string>
#include <functional>
#include <memory>

// Zigbee 适配器占位接口
// 当前仅提供框架，不影响现有 MQTT 主链路
// 后续可接入 Zigbee 协调器（如 CC2531）实现 Zigbee 设备通信

class Logger;

class ZigbeeAdapter {
public:
    using MessageHandler = std::function<void(const std::string& device_id,
                                               const std::string& payload)>;

    struct Options {
        std::string serial_port;   // Zigbee 协调器串口，如 /dev/ttyUSB0
        int baud_rate = 115200;
        bool enabled = false;
    };

    explicit ZigbeeAdapter(std::shared_ptr<Logger> logger);
    ~ZigbeeAdapter();

    // 连接 Zigbee 协调器
    bool Connect(const Options& opts);

    // 断开连接
    void Disconnect();

    // 是否已连接
    bool IsConnected() const;

    // 发送命令到 Zigbee 设备
    bool SendCommand(const std::string& device_id, const std::string& payload);

    // 设置消息回调（收到 Zigbee 设备数据时调用）
    void SetMessageHandler(MessageHandler handler);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

#endif
