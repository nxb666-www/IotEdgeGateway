#include "rest_api.hpp"
#include "core/storage/sqlite_storage.hpp"
#include <utility>

// 静态成员初始化
DeviceRegistry* RestApi::registry_ = nullptr;
MqttClient* RestApi::mqtt_client_ = nullptr;
SqliteStorage* RestApi::sqlite_ = nullptr;
std::function<void(const std::string&)> RestApi::manual_control_callback_;

void RestApi::SetRegistry(DeviceRegistry* r) {
    registry_ = r;
}

void RestApi::SetMqttClient(MqttClient* client) {
    mqtt_client_ = client;
}

void RestApi::SetSqliteStorage(SqliteStorage* db) {
    sqlite_ = db;
}

void RestApi::SetManualControlCallback(std::function<void(const std::string&)> cb) {
    manual_control_callback_ = std::move(cb);
}

// 路由分发入口
bool RestApi::HandleRequest(mg_connection* c, mg_http_message* msg) {
    struct mg_str caps[2];

    if (mg_match(msg->uri, mg_str("/api/health"), NULL)) {
        mg_http_reply(c, 200, "Content-Type: application/json\r\n",
            "{\"status\":\"正常\"}\n");

    } else if (mg_match(msg->uri, mg_str("/api/version"), NULL)) {
        mg_http_reply(c, 200, "Content-Type: application/json\r\n",
            "{\"version\":\"%s\"}\n", IOTGW_VERSION);

    } else if (mg_match(msg->uri, mg_str("/api/devices/#"), caps)) {
        std::string device_id(caps[0].buf, caps[0].len);
        DeviceApi::HandleDeviceDetail(c, device_id);

    } else if (mg_match(msg->uri, mg_str("/api/devices"), NULL)) {
        DeviceApi::HandleDeviceList(c);

    } else if (mg_match(msg->uri, mg_str("/api/actuators/#/set"), caps)) {
        std::string actuator_id(caps[0].buf, caps[0].len);
        DeviceApi::HandleActuatorSet(c, msg, actuator_id);

    } else if (mg_match(msg->uri, mg_str("/api/status"), NULL)) {
        HandleStatus(c);

    } else if (mg_match(msg->uri, mg_str("/api/control"), NULL)) {
        HandleControl(c, msg);

    } else if (mg_match(msg->uri, mg_str("/api/camera/start_stream"), NULL)) {
        CameraApi::HandleStartStream(c);

    } else if (mg_match(msg->uri, mg_str("/api/camera/stop_stream"), NULL)) {
        CameraApi::HandleStopStream(c);

    } else if (mg_match(msg->uri, mg_str("/api/camera/snapshot"), NULL)) {
        CameraApi::HandleSnapshot(c);

    } else if (mg_match(msg->uri, mg_str("/api/camera/start_record"), NULL)) {
        CameraApi::HandleStartRecord(c);

    } else if (mg_match(msg->uri, mg_str("/api/camera/stop_record"), NULL)) {
        CameraApi::HandleStopRecord(c);

    } else if (mg_match(msg->uri, mg_str("/api/camera/probe"), NULL)) {
        CameraApi::HandleProbe(c);

    } else if (mg_match(msg->uri, mg_str("/stream/live.jpg"), NULL)) {
        CameraApi::HandleLiveJpg(c);

    } else if (mg_match(msg->uri, mg_str("/api/rules/reload"), NULL)) {
        RuleApi::HandleRuleReload(c);

    } else if (mg_match(msg->uri, mg_str("/api/rules/#/enable"), caps)) {
        std::string rule_id(caps[0].buf, caps[0].len);
        RuleApi::HandleRuleEnable(c, rule_id);

    } else if (mg_match(msg->uri, mg_str("/api/rules/#/disable"), caps)) {
        std::string rule_id(caps[0].buf, caps[0].len);
        RuleApi::HandleRuleDisable(c, rule_id);

    } else if (mg_match(msg->uri, mg_str("/api/rules"), NULL)) {
        RuleApi::HandleRuleList(c);

    } else if (mg_match(msg->uri, mg_str("/api/history"), NULL)) {
        HandleHistory(c, msg);

    } else {
        return false;
    }
    return true;
}

// GET /api/status — 设备状态聚合
void RestApi::HandleStatus(mg_connection* c) {
    if (!registry_) {
        mg_http_reply(c, 500, "Content-Type: application/json\r\n",
            "{\"error\":\"设备注册表未初始化\"}\n");
        return;
    }

    auto devices = registry_->List();
    std::string json = "{";
    bool first = true;

    for (const auto& dev : devices) {
        if (!first) json += ",";
        first = false;
        // 用 json_utils 提取 value 字段
        std::string value = json_utils::ExtractValue(dev.status.last_payload);
        json += "\"" + dev.id + "\":" + value;
    }

    json += "}\n";
    mg_http_reply(c, 200, "Content-Type: application/json\r\n",
        "%s", json.c_str());
}

// POST /api/control — 下发控制指令
void RestApi::HandleControl(mg_connection* c, mg_http_message* msg) {
    if (!mqtt_client_ || !mqtt_client_->IsOpen()) {
        mg_http_reply(c, 503, "Content-Type: application/json\r\n",
            "{\"error\":\"MQTT未连接\"}\n");
        return;
    }

    if (!registry_) {
        mg_http_reply(c, 500, "Content-Type: application/json\r\n",
            "{\"error\":\"设备注册表未初始化\"}\n");
        return;
    }

    std::string body(msg->body.buf, msg->body.len);

    // 提取 payload（兼容两种格式）
    std::string payload_str = body;
    size_t payload_pos = body.find("\"payload\"");
    if (payload_pos != std::string::npos) {
        size_t start = body.find('{', payload_pos);
        if (start != std::string::npos) {
            int depth = 0;
            size_t end = start;
            for (size_t i = start; i < body.length(); i++) {
                if (body[i] == '{') depth++;
                if (body[i] == '}') depth--;
                if (depth == 0) { end = i; break; }
            }
            payload_str = body.substr(start, end - start + 1);
        }
    }

    // 用 json_utils 提取各个控制参数
    std::string p = "{\"type\":\"control\",\"payload\":" + payload_str + "}";
    if (manual_control_callback_) {
        manual_control_callback_(payload_str);
    }
    mqtt_client_->Publish("iotgw/dev/cmd/control", p);

    mg_http_reply(c, 200, "Content-Type: application/json\r\n",
        "{\"status\":\"正常\"}\n");
}

// GET /api/history?device_id=xxx&limit=100 — 历史遥测数据查询
void RestApi::HandleHistory(mg_connection* c, mg_http_message* msg) {
    if (!sqlite_ || !sqlite_->IsOpen()) {
        mg_http_reply(c, 503, "Content-Type: application/json\r\n",
            "{\"error\":\"SQLite 未启用\"}\n");
        return;
    }

    // 解析查询参数
    char device_id_buf[128] = {0};
    char limit_buf[16] = {0};
    char from_buf[32] = {0};
    char to_buf[32] = {0};
    mg_http_get_var(&msg->query, "device_id", device_id_buf, sizeof(device_id_buf));
    mg_http_get_var(&msg->query, "limit", limit_buf, sizeof(limit_buf));
    mg_http_get_var(&msg->query, "from", from_buf, sizeof(from_buf));
    mg_http_get_var(&msg->query, "to", to_buf, sizeof(to_buf));

    if (device_id_buf[0] == '\0') {
        mg_http_reply(c, 400, "Content-Type: application/json\r\n",
            "{\"error\":\"缺少 device_id 参数\"}\n");
        return;
    }

    int limit = 100;
    if (limit_buf[0] != '\0') limit = atoi(limit_buf);
    int64_t from_ms = 0, to_ms = 0;
    if (from_buf[0] != '\0') from_ms = (int64_t)atoll(from_buf);
    if (to_buf[0] != '\0') to_ms = (int64_t)atoll(to_buf);

    std::string json = sqlite_->QueryHistoryJson(device_id_buf, limit, from_ms, to_ms);
    mg_http_reply(c, 200, "Content-Type: application/json\r\n",
        "%s", json.c_str());
}
