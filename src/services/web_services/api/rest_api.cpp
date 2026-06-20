#include "rest_api.hpp"

// ========== 静态成员初始化 ==========
// 静态成员必须在 .cpp 里定义一次，否则链接报错
DeviceRegistry* RestApi::registry_ = nullptr;
MqttClient* RestApi::mqtt_client_ = nullptr;

// 设置设备注册表指针
// 在 gateway_core.cpp 启动时调用，把注册表注入进来
void RestApi::SetRegistry(DeviceRegistry* r) {
    registry_ = r;
}

// 设置 MQTT 客户端指针
// 在 gateway_core.cpp 启动时调用，把 MQTT 客户端注入进来
void RestApi::SetMqttClient(MqttClient* client) {
    mqtt_client_ = client;
}

// ========== 路由分发入口 ==========
// 所有 HTTP 请求都从这里进来
// 根据 URL 路径判断该调哪个处理函数
// 返回 true = 已处理，false = 不是 API 请求（交给静态文件服务）
bool RestApi::HandleRequest(mg_connection* c, mg_http_message* msg) {
    struct mg_str caps[2];  // 存 URL 通配符捕获的内容

    // /api/health — 健康检查
    // 浏览器或运维工具用来检测网关是否活着
    if (mg_match(msg->uri, mg_str("/api/health"), NULL)) {
        mg_http_reply(c, 200, "Content-Type: application/json\r\n",
            "{\"status\":\"正常\"}\n");

    // /api/version — 版本查询
    // IOTGW_VERSION 是 CMake 从 version.hpp.in 自动生成的
    } else if (mg_match(msg->uri, mg_str("/api/version"), NULL)) {
        mg_http_reply(c, 200, "Content-Type: application/json\r\n",
            "{\"version\":\"%s\"}\n", IOTGW_VERSION);

    // /api/devices/:id — 设备详情
    // # 是通配符，匹配的内容存到 caps[0]
    // 比如访问 /api/devices/temp，caps[0] 就是 "temp"
    } else if (mg_match(msg->uri, mg_str("/api/devices/#"), caps)) {
        std::string device_id(caps[0].buf, caps[0].len);
        DeviceApi::HandleDeviceDetail(c, device_id);

    // /api/devices — 设备列表
    // 注意：必须放在 /api/devices/# 后面，否则会先匹配到这个
    } else if (mg_match(msg->uri, mg_str("/api/devices"), NULL)) {
        DeviceApi::HandleDeviceList(c);

    // /api/actuators/:id/set — 下发执行器命令
    // 比如 /api/actuators/led/set → 控制 LED
    } else if (mg_match(msg->uri, mg_str("/api/actuators/#/set"), caps)) {
        std::string actuator_id(caps[0].buf, caps[0].len);
        DeviceApi::HandleActuatorSet(c, msg, actuator_id);

    // /api/status — 设备状态聚合
    // 前端每秒轮询这个接口，获取所有传感器最新数据
    } else if (mg_match(msg->uri, mg_str("/api/status"), NULL)) {
        HandleStatus(c);

    // /api/control — 下发控制指令
    // 前端控制面板发 LED/电机/蜂鸣器命令到这里
    } else if (mg_match(msg->uri, mg_str("/api/control"), NULL)) {
        HandleControl(c, msg);

    // /api/rules/reload — 重新加载规则配置文件
    // 改了 YAML 规则文件后，不用重启就能重新加载
    } else if (mg_match(msg->uri, mg_str("/api/rules/reload"), NULL)) {
        RuleApi::HandleRuleReload(c);

    // /api/rules/:id/enable — 启用规则
    } else if (mg_match(msg->uri, mg_str("/api/rules/#/enable"), caps)) {
        std::string rule_id(caps[0].buf, caps[0].len);
        RuleApi::HandleRuleEnable(c, rule_id);

    // /api/rules/:id/disable — 禁用规则
    } else if (mg_match(msg->uri, mg_str("/api/rules/#/disable"), caps)) {
        std::string rule_id(caps[0].buf, caps[0].len);
        RuleApi::HandleRuleDisable(c, rule_id);

    // /api/rules — 规则列表
    } else if (mg_match(msg->uri, mg_str("/api/rules"), NULL)) {
        RuleApi::HandleRuleList(c);

    // 都不匹配 → 不是 API 请求，返回 false 让静态文件服务处理
    } else {
        return false;
    }
    return true;
}

// ========== GET /api/status ==========
// 设备状态聚合：遍历所有设备，取每个设备的最新数据
// 前端每秒轮询这个接口，显示传感器数据
// 返回格式: {"temp":25.5,"humi":60.0,"light":500,"led_on":1,...}
void RestApi::HandleStatus(mg_connection* c) {
    if (!registry_) {
        mg_http_reply(c, 500, "Content-Type: application/json\r\n",
            "{\"error\":\"设备注册表未初始化\"}\n");
        return;
    }

    // 取所有设备
    auto devices = registry_->List();
    std::string json = "{";
    bool first = true;

    for (const auto& dev : devices) {
        if (!first) json += ",";
        first = false;

        // 解析 last_payload 中的 value 字段
        // last_payload 可能是两种格式：
        //   简单格式: {"value":25.5}
        //   信封格式: {"device_id":"temp","type":"sensor","data":{"value":25.5},"ts":1700000000}
        std::string value = ExtractValue(dev.status.last_payload);
        json += "\"" + dev.id + "\":" + value;
    }

    json += "}\n";
    mg_http_reply(c, 200, "Content-Type: application/json\r\n",
        "%s", json.c_str());
}

// ========== POST /api/control ==========
// 前端控制面板发控制命令到这里
// 请求格式: {"type":"control","payload":{"led_on":1,"led_br":80,"motor_on":1,...}}
// 处理后通过 MQTT 下发给对应的执行器
void RestApi::HandleControl(mg_connection* c, mg_http_message* msg) {
    // 检查 MQTT 是否连接
    if (!mqtt_client_ || !mqtt_client_->IsOpen()) {
        mg_http_reply(c, 503, "Content-Type: application/json\r\n",
            "{\"error\":\"MQTT未连接\"}\n");
        return;
    }

    // 检查注册表是否可用
    if (!registry_) {
        mg_http_reply(c, 500, "Content-Type: application/json\r\n",
            "{\"error\":\"设备注册表未初始化\"}\n");
        return;
    }

    // 读取请求体
    std::string body(msg->body.buf, msg->body.len);

    // 提取 payload 里的字段
    // 前端发的格式可能是 {"type":"control","payload":{...}}（有外层包裹）
    // 也可能是直接 {"led_on":1,...}（没有外层包裹）
    // 两种都兼容
    std::string payload_str = body;
    size_t payload_pos = body.find("\"payload\"");
    if (payload_pos != std::string::npos) {
        // 找到 "payload": 后面的 {...}
        size_t start = body.find('{', payload_pos);
        if (start != std::string::npos) {
            // 用括号匹配找到完整的 JSON 对象
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

    // 提取各个控制参数
    int led_on = GetJsonInt(payload_str, "led_on");     // LED 开关
    int led_br = GetJsonInt(payload_str, "led_br");     // LED 亮度
    int motor_on = GetJsonInt(payload_str, "motor_on"); // 电机开关
    int motor_sp = GetJsonInt(payload_str, "motor_sp"); // 电机速度
    int motor_dir = GetJsonInt(payload_str, "motor_dir");// 电机方向
    int buzzer = GetJsonInt(payload_str, "buzzer");     // 蜂鸣器

    std::string topic;

    // LED 命令：查注册表找 command_topic，通过 MQTT 发布
    if (registry_->GetCommandTopic("led", topic)) {
        std::string p = "{\"on\":" + std::to_string(led_on)
                      + ",\"br\":" + std::to_string(led_br) + "}";
        mqtt_client_->Publish(topic, p);
    }

    // 电机命令
    if (registry_->GetCommandTopic("motor", topic)) {
        std::string p = "{\"on\":" + std::to_string(motor_on)
                      + ",\"sp\":" + std::to_string(motor_sp)
                      + ",\"dir\":" + std::to_string(motor_dir) + "}";
        mqtt_client_->Publish(topic, p);
    }

    // 蜂鸣器命令
    if (registry_->GetCommandTopic("buzzer", topic)) {
        std::string p = "{\"on\":" + std::to_string(buzzer) + "}";
        mqtt_client_->Publish(topic, p);
    }

    // 返回成功
    mg_http_reply(c, 200, "Content-Type: application/json\r\n",
        "{\"status\":\"正常\"}\n");
}

// ========== JSON 工具函数 ==========

// 从 JSON 字符串中提取 value 字段的值
// 支持两种格式：
//   简单格式: {"value":25.5} → 返回 "25.5"
//   信封格式: {"data":{"value":25.5}} → 返回 "25.5"
// 找不到就返回原始 JSON 字符串
std::string RestApi::ExtractValue(const std::string& json) {
    // 先尝试信封格式：找 "data" 对象里的 "value"
    size_t data_pos = json.find("\"data\"");
    if (data_pos != std::string::npos) {
        size_t val_pos = json.find("\"value\"", data_pos);
        if (val_pos != std::string::npos) {
            return ExtractNumber(json, val_pos);
        }
    }

    // 再尝试简单格式：直接找 "value"
    size_t val_pos = json.find("\"value\"");
    if (val_pos != std::string::npos) {
        return ExtractNumber(json, val_pos);
    }

    // 找不到就返回原始数据
    if (json.empty()) return "\"\"";
    return json;
}

// 从 JSON 字符串中指定位置提取数字值
// 从 "value":25.5 中提取 25.5
// key_pos 是 "value" 的位置
std::string RestApi::ExtractNumber(const std::string& json, size_t key_pos) {
    // 找到冒号
    size_t colon = json.find(':', key_pos);
    if (colon == std::string::npos) return "0";

    // 跳过空格
    size_t start = colon + 1;
    while (start < json.length() && json[start] == ' ') start++;

    // 读取数字（包括小数点和负号）
    size_t end = start;
    if (end < json.length() && json[end] == '-') end++;
    while (end < json.length() &&
           ((json[end] >= '0' && json[end] <= '9') || json[end] == '.')) {
        end++;
    }

    if (end == start) return "0";
    return json.substr(start, end - start);
}

// 从 JSON 字符串中提取整数值
// 查找 "key":number 格式，返回数值
// 比如 GetJsonInt("{\"led_on\":1}", "led_on") → 返回 1
int RestApi::GetJsonInt(const std::string& json, const std::string& key) {
    // 先找 "key" 字符串的位置
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return 0;  // 找不到返回 0

    // 找到冒号
    pos = json.find(':', pos + search.length());
    if (pos == std::string::npos) return 0;
    pos++;

    // 跳过空格
    while (pos < json.length() && json[pos] == ' ') pos++;

    // 处理负数
    bool negative = false;
    if (pos < json.length() && json[pos] == '-') {
        negative = true;
        pos++;
    }

    // 读取数字
    int value = 0;
    while (pos < json.length() && json[pos] >= '0' && json[pos] <= '9') {
        value = value * 10 + (json[pos] - '0');
        pos++;
    }

    return negative ? -value : value;
}
