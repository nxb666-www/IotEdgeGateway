#ifndef MQTT_ADAPTER_HPP
#define MQTT_ADAPTER_HPP

#include <string>
#include <functional>
#include <memory>
#include <cstdint>
#include "mongoose.h"
#include "core/common/logger/logger.hpp"

// MQTT 客户端
// 用法：
//   MqttClient client(mgr, logger);
//   MqttClient::Options opt;
//   opt.url = "mqtt://localhost:1883";
//   opt.client_id = "iotgw-dev";
//   client.Connect(opt);
//   client.Subscribe("iotgw/dev/telemetry/#");
//   client.Publish("iotgw/dev/cmd/led", "{\"value\":1}");
class MqttClient {
    // 声明回调函数为友元，这样它能访问私有成员
    friend void mqtt_event_handler(struct mg_connection*, int, void*);

public:
    // 连接选项
    struct Options {
        std::string url;              // "mqtt://host:port"
        std::string client_id;
        std::string user;
        std::string pass;
        uint16_t keepalive_sec = 30;
        bool clean_session = true;
        uint8_t version = 4;
    };

    // 消息处理回调类型
    using MessageHandler = std::function<void(const std::string& topic,
                                               const std::string& payload)>;

    explicit MqttClient(struct mg_mgr* mgr,
                        std::shared_ptr<Logger> logger);

    bool Connect(const Options& opt);
    bool Subscribe(const std::string& topic, uint8_t qos = 0);
    bool Publish(const std::string& topic, const std::string& payload,
                 uint8_t qos = 0, bool retain = false);
    void SetMessageHandler(MessageHandler handler);
    bool IsOpen() const;

private:
    struct mg_mgr* mgr_;                    // Mongoose 管理器
    std::shared_ptr<Logger> logger_;        // 日志器
    struct mg_connection* conn_;            // MQTT 连接
    bool connected_;                        // 是否已连接
    Options options_;                       // 连接选项（重连用）
    MessageHandler message_handler_;        // 消息处理回调
};

#endif
