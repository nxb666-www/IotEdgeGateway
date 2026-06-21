#ifndef MQTT_ADAPTER_HPP
#define MQTT_ADAPTER_HPP

#include <string>
#include <functional>
#include <memory>
#include <cstdint>
#include "mongoose.h"
#include "core/common/logger/logger.hpp"

// MQTT 瀹㈡埛绔?// 鐢ㄦ硶锛?//   MqttClient client(mgr, logger);
//   MqttClient::Options opt;
//   opt.url = "mqtt://localhost:1883";
//   opt.client_id = "iotgw-dev";
//   client.Connect(opt);
//   client.Subscribe("iotgw/dev/telemetry/#");
//   client.Publish("iotgw/dev/cmd/led", "{\"value\":1}");
class MqttClient {
    // 澹版槑鍥炶皟鍑芥暟涓哄弸鍏冿紝杩欐牱瀹冭兘璁块棶绉佹湁鎴愬憳
    friend void mqtt_event_handler(struct mg_connection*, int, void*);

public:
    // 杩炴帴閫夐」
    struct Options {
        std::string url;              // "mqtt://host:port"
        std::string client_id;
        std::string user;
        std::string pass;
        uint16_t keepalive_sec = 30;
        bool clean_session = true;
        uint8_t version = 4;
    };

    // 娑堟伅澶勭悊鍥炶皟绫诲瀷
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
    struct mg_mgr* mgr_;                    // Mongoose 绠＄悊鍣?    std::shared_ptr<Logger> logger_;        // 鏃ュ織鍣?    struct mg_connection* conn_;            // MQTT 杩炴帴
    bool connected_;                        // 鏄惁宸茶繛鎺?    Options options_;                       // 杩炴帴閫夐」锛堥噸杩炵敤锛?    MessageHandler message_handler_;        // 娑堟伅澶勭悊鍥炶皟
};

#endif
