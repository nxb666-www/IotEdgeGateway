#ifndef MQTT_ADAPTER_HPP
#define MQTT_ADAPTER_HPP

#include <string>
#include <functional>
#include <memory>
#include <cstdint>
#include "mongoose.h"
#include "core/common/logger/logger.hpp"

class MqttClient {
    friend void mqtt_event_handler(struct mg_connection*, int, void*);

public:
    struct Options {
        std::string url;
        std::string client_id;
        std::string user;
        std::string pass;
        uint16_t keepalive_sec = 0;
        bool clean_session = true;
        uint8_t version = 4;
    };

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
    struct mg_mgr* mgr_;
    std::shared_ptr<Logger> logger_;
    struct mg_connection* conn_;
    bool connected_;
    Options options_;
    MessageHandler message_handler_;
};

#endif
