#ifndef REST_API_HPP
#define REST_API_HPP

#include "camera_api.hpp"
#include "core/common/utils/json_utils.hpp"
#include "device_api.hpp"
#include "mongoose.h"
#include "rule_api.hpp"
#include "version.hpp"

#include <functional>
#include <string>

class SqliteStorage;

class ZigbeeAdapter;

class RestApi {
private:
    static DeviceRegistry* registry_;
    static MqttClient* mqtt_client_;
    static SqliteStorage* sqlite_;
    static ZigbeeAdapter* zigbee_adapter_;
    static std::function<void(const std::string&)> manual_control_callback_;

public:
    static void SetRegistry(DeviceRegistry* r);
    static void SetMqttClient(MqttClient* client);
    static void SetSqliteStorage(SqliteStorage* db);
    static void SetZigbeeAdapter(ZigbeeAdapter* adapter);
    static void SetManualControlCallback(std::function<void(const std::string&)> cb);

    static bool HandleRequest(mg_connection* c, mg_http_message* msg);

private:
    static void HandleStatus(mg_connection* c);
    static void HandleControl(mg_connection* c, mg_http_message* msg);
    static void HandleHistory(mg_connection* c, mg_http_message* msg);
    static void HandleTransport(mg_connection* c, mg_http_message* msg);
};

#endif
