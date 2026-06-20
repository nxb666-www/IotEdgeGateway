#include "device_manager.hpp"

// 从 MQTT 主题提取设备ID
// 例: "iotgw/dev/telemetry/temp" → "temp"
static std::string ExtractDeviceId(const std::string& topic) {
    auto pos = topic.rfind('/');
    if (pos != std::string::npos && pos + 1 < topic.size()) {
        return topic.substr(pos + 1);
    }
    return topic;
}

static std::string JsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char ch : s) {
        switch (ch) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += ch; break;
        }
    }
    return out;
}

bool DeviceRegistry::Register(DeviceEntity device) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (device.id.empty()) return false;

    by_id_[device.id] = device;

    // 建立主题 → 设备ID的映射
    if (!device.telemetry_topic.empty()) {
        telemetry_topic_to_id_[device.telemetry_topic] = device.id;
    }
    if (!device.command_topic.empty()) {
        command_topic_to_id_[device.command_topic] = device.id;
    }
    return true;
}

bool DeviceRegistry::Has(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return by_id_.count(id) > 0;
}

bool DeviceRegistry::Get(const std::string& id, DeviceEntity& out) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = by_id_.find(id);
    if (it == by_id_.end()) return false;
    out = it->second;
    return true;
}

std::vector<DeviceEntity> DeviceRegistry::List() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<DeviceEntity> result;
    for (const auto& pair : by_id_) {
        result.push_back(pair.second);
    }
    return result;
}

bool DeviceRegistry::UpdateFromTelemetryTopic(const std::string& topic,
                                               const std::string& payload,
                                               int64_t now_ms,
                                               std::string& out_device_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = telemetry_topic_to_id_.find(topic);
    if (it == telemetry_topic_to_id_.end()) return false;

    out_device_id = it->second;
    auto dev_it = by_id_.find(out_device_id);
    if (dev_it == by_id_.end()) return false;

    dev_it->second.status.online = true;
    dev_it->second.status.last_seen_ms = now_ms;
    dev_it->second.status.last_payload = payload;
    dev_it->second.status.last_topic = topic;
    return true;
}

bool DeviceRegistry::UpsertMqttDeviceFromTopic(const std::string& topic,
                                                const std::string& payload,
                                                int64_t now_ms,
                                                std::string& out_device_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 先通过主题查找已注册的设备
    auto it = telemetry_topic_to_id_.find(topic);
    if (it != telemetry_topic_to_id_.end()) {
        out_device_id = it->second;
        auto dev_it = by_id_.find(out_device_id);
        if (dev_it != by_id_.end()) {
            dev_it->second.status.online = true;
            dev_it->second.status.last_seen_ms = now_ms;
            dev_it->second.status.last_payload = payload;
            dev_it->second.status.last_topic = topic;
            return true;
        }
    }

    // 设备未注册，自动创建
    out_device_id = ExtractDeviceId(topic);
    if (out_device_id.empty()) return false;

    DeviceEntity dev;
    dev.id = out_device_id;
    dev.kind = "unknown";
    dev.transport = "mqtt";
    dev.telemetry_topic = topic;
    dev.status.online = true;
    dev.status.last_seen_ms = now_ms;
    dev.status.last_payload = payload;
    dev.status.last_topic = topic;

    by_id_[out_device_id] = dev;
    telemetry_topic_to_id_[topic] = out_device_id;
    return true;
}

bool DeviceRegistry::GetCommandTopic(const std::string& device_id, std::string& out_topic) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = by_id_.find(device_id);
    if (it == by_id_.end()) return false;
    out_topic = it->second.command_topic;
    return !out_topic.empty();
}

bool DeviceRegistry::GetTelemetryTopic(const std::string& device_id, std::string& out_topic) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = by_id_.find(device_id);
    if (it == by_id_.end()) return false;
    out_topic = it->second.telemetry_topic;
    return !out_topic.empty();
}

std::string DeviceRegistry::ToJsonList() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string json = "[";
    bool first = true;
    for (const auto& pair : by_id_) {
        if (!first) json += ",";
        first = false;

        const auto& dev = pair.second;
        json += "{";
        json += "\"id\":\"" + JsonEscape(dev.id) + "\",";
        json += "\"kind\":\"" + JsonEscape(dev.kind) + "\",";
        json += "\"transport\":\"" + JsonEscape(dev.transport) + "\",";
        json += "\"telemetry_topic\":\"" + JsonEscape(dev.telemetry_topic) + "\",";
        json += "\"command_topic\":\"" + JsonEscape(dev.command_topic) + "\",";
        json += "\"status\":{";
        json += "\"online\":" + std::string(dev.status.online ? "true" : "false") + ",";
        json += "\"last_seen_ms\":" + std::to_string(dev.status.last_seen_ms) + ",";
        json += "\"last_topic\":\"" + JsonEscape(dev.status.last_topic) + "\",";
        json += "\"last_payload\":\"" + JsonEscape(dev.status.last_payload) + "\"";
        json += "}";
        json += "}";
    }
    json += "]";
    return json;
}

bool DeviceRegistry::ToJsonOne(const std::string& id, std::string& out_json) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = by_id_.find(id);
    if (it == by_id_.end()) return false;

    const auto& dev = it->second;
    out_json = "{";
    out_json += "\"id\":\"" + JsonEscape(dev.id) + "\",";
    out_json += "\"kind\":\"" + JsonEscape(dev.kind) + "\",";
    out_json += "\"transport\":\"" + JsonEscape(dev.transport) + "\",";
    out_json += "\"telemetry_topic\":\"" + JsonEscape(dev.telemetry_topic) + "\",";
    out_json += "\"command_topic\":\"" + JsonEscape(dev.command_topic) + "\",";
    out_json += "\"status\":{";
    out_json += "\"online\":" + std::string(dev.status.online ? "true" : "false") + ",";
    out_json += "\"last_seen_ms\":" + std::to_string(dev.status.last_seen_ms) + ",";
    out_json += "\"last_topic\":\"" + JsonEscape(dev.status.last_topic) + "\",";
    out_json += "\"last_payload\":\"" + JsonEscape(dev.status.last_payload) + "\"";
    out_json += "}";
    out_json += "}";
    return true;
}
