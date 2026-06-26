#include "rest_api.hpp"

#include "core/storage/sqlite_storage.hpp"

#include <cstdlib>
#include <utility>

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

bool RestApi::HandleRequest(mg_connection* c, mg_http_message* msg) {
    struct mg_str caps[2];

    if (mg_match(msg->uri, mg_str("/api/health"), NULL)) {
        mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"status\":\"ok\"}\n");
    } else if (mg_match(msg->uri, mg_str("/api/version"), NULL)) {
        mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"version\":\"%s\"}\n", IOTGW_VERSION);
    } else if (mg_match(msg->uri, mg_str("/api/devices/#"), caps)) {
        DeviceApi::HandleDeviceDetail(c, std::string(caps[0].buf, caps[0].len));
    } else if (mg_match(msg->uri, mg_str("/api/devices"), NULL)) {
        DeviceApi::HandleDeviceList(c);
    } else if (mg_match(msg->uri, mg_str("/api/actuators/#/set"), caps)) {
        DeviceApi::HandleActuatorSet(c, msg, std::string(caps[0].buf, caps[0].len));
    } else if (mg_match(msg->uri, mg_str("/api/status"), NULL)) {
        HandleStatus(c);
    } else if (mg_match(msg->uri, mg_str("/api/control"), NULL)) {
        HandleControl(c, msg);
    } else if (mg_match(msg->uri, mg_str("/api/transport"), NULL)) {
        HandleTransport(c, msg);
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
    } else if (mg_match(msg->uri, mg_str("/api/camera/stream"), NULL)) {
        CameraApi::HandleMjpegStream(c);
    } else if (mg_match(msg->uri, mg_str("/api/camera/photos"), NULL)) {
        CameraApi::HandleListPhotos(c);
    } else if (mg_match(msg->uri, mg_str("/api/camera/videos"), NULL)) {
        CameraApi::HandleListVideos(c);
    } else if (mg_match(msg->uri, mg_str("/api/camera/logs"), NULL)) {
        CameraApi::HandleListLogs(c);
    } else if (mg_match(msg->uri, mg_str("/api/camera/delete_photo/#"), caps)) {
        CameraApi::HandleDeletePhoto(c, std::string(caps[0].buf, caps[0].len));
    } else if (mg_match(msg->uri, mg_str("/api/camera/delete_video/#"), caps)) {
        CameraApi::HandleDeleteVideo(c, std::string(caps[0].buf, caps[0].len));
    } else if (mg_match(msg->uri, mg_str("/api/camera/clear_logs"), NULL)) {
        CameraApi::HandleClearLogs(c);
    } else if (mg_match(msg->uri, mg_str("/data/media/photos/#"), caps)) {
        CameraApi::HandlePhotoFile(c, std::string(caps[0].buf, caps[0].len));
    } else if (mg_match(msg->uri, mg_str("/data/media/videos/#"), caps)) {
        CameraApi::HandleVideoFile(c, std::string(caps[0].buf, caps[0].len));
    } else if (mg_match(msg->uri, mg_str("/api/camera/logs/#"), caps)) {
        CameraApi::HandleLogFile(c, std::string(caps[0].buf, caps[0].len));
    } else if (mg_match(msg->uri, mg_str("/stream/live.jpg"), NULL)) {
        CameraApi::HandleLiveJpg(c);
    } else if (mg_match(msg->uri, mg_str("/api/rules/reload"), NULL)) {
        RuleApi::HandleRuleReload(c);
    } else if (mg_match(msg->uri, mg_str("/api/rules/#/enable"), caps)) {
        RuleApi::HandleRuleEnable(c, std::string(caps[0].buf, caps[0].len));
    } else if (mg_match(msg->uri, mg_str("/api/rules/#/disable"), caps)) {
        RuleApi::HandleRuleDisable(c, std::string(caps[0].buf, caps[0].len));
    } else if (mg_match(msg->uri, mg_str("/api/rules"), NULL)) {
        RuleApi::HandleRuleList(c);
    } else if (mg_match(msg->uri, mg_str("/api/history"), NULL)) {
        HandleHistory(c, msg);
    } else {
        return false;
    }
    return true;
}

void RestApi::HandleStatus(mg_connection* c) {
    if (!registry_) {
        mg_http_reply(c, 500, "Content-Type: application/json\r\n",
            "{\"error\":\"device_registry_not_ready\"}\n");
        return;
    }

    auto devices = registry_->List();
    std::string json = "{";
    bool first = true;
    for (const auto& dev : devices) {
        if (!first) json += ",";
        first = false;
        json += json_utils::Quote(dev.id) + ":" + json_utils::ExtractValue(dev.status.last_payload);
    }
    json += "}\n";
    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", json.c_str());
}

void RestApi::HandleControl(mg_connection* c, mg_http_message* msg) {
    if (!mqtt_client_ || !mqtt_client_->IsOpen()) {
        mg_http_reply(c, 503, "Content-Type: application/json\r\n",
            "{\"ok\":false,\"error\":\"mqtt_not_connected\",\"stage\":\"gateway\"}\n");
        return;
    }

    std::string body(msg->body.buf, msg->body.len);
    std::string payload_str = body;

    size_t payload_pos = body.find("\"payload\"");
    if (payload_pos != std::string::npos) {
        size_t start = body.find('{', payload_pos);
        if (start != std::string::npos) {
            int depth = 0;
            size_t end = start;
            for (size_t i = start; i < body.length(); ++i) {
                if (body[i] == '{') depth++;
                if (body[i] == '}') depth--;
                if (depth == 0) {
                    end = i;
                    break;
                }
            }
            payload_str = body.substr(start, end - start + 1);
        }
    }

    // 先记录手动控制优先级，再发布 MQTT
    if (manual_control_callback_) {
        manual_control_callback_(payload_str);
    }

    std::string p = "{\"type\":\"control\",\"payload\":" + payload_str + "}";
    bool published = mqtt_client_->Publish("iotgw/dev/cmd/control", p);
    if (published) {
        mqtt_client_->Publish("iotgw/dev/cmd/control", p);
    }

    if (published) {
        mg_http_reply(c, 200, "Content-Type: application/json\r\n",
            "{\"ok\":true,\"stage\":\"mqtt_published\",\"topic\":\"iotgw/dev/cmd/control\",\"repeat\":2,\"payload\":%s}\n",
            p.c_str());
    } else {
        mg_http_reply(c, 500, "Content-Type: application/json\r\n",
            "{\"ok\":false,\"error\":\"mqtt_publish_failed\",\"stage\":\"mqtt\"}\n");
    }
}

void RestApi::HandleHistory(mg_connection* c, mg_http_message* msg) {
    if (!sqlite_ || !sqlite_->IsOpen()) {
        mg_http_reply(c, 503, "Content-Type: application/json\r\n",
            "{\"error\":\"sqlite_not_enabled\"}\n");
        return;
    }

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
            "{\"error\":\"missing_device_id\"}\n");
        return;
    }

    int limit = 100;
    if (limit_buf[0] != '\0') limit = std::atoi(limit_buf);
    int64_t from_ms = 0;
    int64_t to_ms = 0;
    if (from_buf[0] != '\0') from_ms = static_cast<int64_t>(std::atoll(from_buf));
    if (to_buf[0] != '\0') to_ms = static_cast<int64_t>(std::atoll(to_buf));

    std::string json = sqlite_->QueryHistoryJson(device_id_buf, limit, from_ms, to_ms);
    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", json.c_str());
}

// GET/POST /api/transport — 通信方式查询/切换
// 当前只有 MQTT 实际可用，Zigbee 为占位
static std::string g_active_transport = "mqtt";

void RestApi::HandleTransport(mg_connection* c, mg_http_message* msg) {
    if (mg_match(msg->method, mg_str("POST"), NULL)) {
        // POST: 切换通信方式
        std::string body(msg->body.buf, msg->body.len);
        size_t pos = body.find("\"transport\"");
        if (pos != std::string::npos) {
            size_t start = body.find('"', pos + 11);
            if (start != std::string::npos) {
                start++;
                size_t end = body.find('"', start);
                std::string val = body.substr(start, end - start);
                if (val == "zigbee") {
                    // Zigbee 当前为占位，不允许切换
                    mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                        "{\"active\":\"mqtt\",\"available\":[\"mqtt\"],\"error\":\"zigbee_not_available\"}\n");
                    return;
                }
                g_active_transport = "mqtt";
            }
        }
    }
    // GET 或切换后返回当前状态
    mg_http_reply(c, 200, "Content-Type: application/json\r\n",
        "{\"active\":\"%s\",\"available\":[\"mqtt\"],\"zigbee\":\"not_connected\"}\n",
        g_active_transport.c_str());
}
