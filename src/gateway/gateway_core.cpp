/**
 * gateway_core.cpp — 网关主程序（iotgw_gateway）
 *
 * 职责：整个系统的中枢
 *
 *   上行接收：
 *     ESP32 发布 MQTT → 本文件订阅 iotgw/dev/telemetry/#
 *     → 更新设备注册表 → 规则引擎检查 → WebSocket 广播给浏览器
 *
 *   下行控制：
 *     浏览器 WebSocket 发命令 → 本文件发布 MQTT 到 iotgw/dev/cmd/#
 *     或规则引擎触发 → 本文件发布 MQTT 到 iotgw/dev/cmd/#
 *     → ESP32 订阅 cmd/# 接收 → 转 STM32 格式 → 串口发给 STM32
 */

#include "gateway_core.hpp"
#include "core/common/config/config_manager.hpp"
#include "core/common/logger/logger.hpp"
#include "core/device/manager/device_manager.hpp"
#include "core/device/protocol_adapters/mqtt_adapter/mqtt_adapter.hpp"
#include "services/web_services/websocket/websocket_server.hpp"
#include "services/web_services/api/rest_api.hpp"
#include "services/web_services/api/device_api.hpp"
#include "services/web_services/api/rule_api.hpp"
#include "core/control/rule_engine.hpp"
#include "mongoose.h"
#include <csignal>
#include <ctime>

static bool running = true;

static void signal_handler(int sig) {
    running = false;
}

static int64_t NowMs() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/**
 * 从配置文件加载设备并注册到设备注册表
 *
 * 读取 config/devices/sensors.yaml 和 actuators.yaml
 * 注册后设备注册表就知道：
 *   - temp 传感器的遥测主题是 iotgw/dev/telemetry/temp
 *   - led 执行器的命令主题是 iotgw/dev/cmd/led
 */
static void LoadDevicesFromConfig(ConfigManager& cfg,
                                   const std::string& topic_prefix,
                                   DeviceRegistry& registry,
                                   std::shared_ptr<Logger> logger) {
    ConfigManager sensors_cfg;
    if (sensors_cfg.LoadYamlFile("config/devices/sensors.yaml")) {
        for (int i = 0; ; i++) {
            std::string prefix = "sensors[" + std::to_string(i) + "].";
            std::string id;
            if (!sensors_cfg.GetString(prefix + "id", id)) break;

            DeviceEntity dev;
            dev.id = id;
            dev.kind = "sensor";
            dev.transport = "mqtt";
            dev.telemetry_topic = topic_prefix + "telemetry/" + id;
            registry.Register(dev);
            logger->Info("注册传感器: " + id);
        }
    }

    ConfigManager actuators_cfg;
    if (actuators_cfg.LoadYamlFile("config/devices/actuators.yaml")) {
        for (int i = 0; ; i++) {
            std::string prefix = "actuators[" + std::to_string(i) + "].";
            std::string id;
            if (!actuators_cfg.GetString(prefix + "id", id)) break;

            DeviceEntity dev;
            dev.id = id;
            dev.kind = "actuator";
            dev.transport = "mqtt";
            dev.command_topic = topic_prefix + "cmd/" + id;
            registry.Register(dev);
            logger->Info("注册执行器: " + id);
        }
    }
}

static void LoadRulesFromFile(const std::string& file_path,
                               const std::string& category,
                               RuleEngine& engine,
                               std::shared_ptr<Logger> logger) {
    ConfigManager cfg;
    if (!cfg.LoadYamlFile(file_path)) {
        logger->Warn("规则文件加载失败: " + file_path);
        return;
    }

    std::string key = category + "_rules";
    for (int i = 0; ; i++) {
        std::string prefix = key + "[" + std::to_string(i) + "].";

        std::string id;
        if (!cfg.GetString(prefix + "id", id)) break;

        bool enabled = true;
        cfg.GetBool(prefix + "enabled", enabled);

        std::string sensor_id, op;
        double value = 0.0;
        cfg.GetString(prefix + "when.sensor_id", sensor_id);
        cfg.GetString(prefix + "when.op", op);
        std::string value_str;
        if (cfg.GetString(prefix + "when.value", value_str)) {
            value = std::stod(value_str);
        }

        std::vector<Action> actions;
        for (int j = 0; ; j++) {
            std::string action_prefix = prefix + "then[" + std::to_string(j) + "].";
            std::string type;
            if (!cfg.GetString(action_prefix + "type", type)) break;

            Action action;
            action.type = type;
            cfg.GetString(action_prefix + "actuator_id", action.actuator_id);
            cfg.GetString(action_prefix + "value", action.value);
            cfg.GetString(action_prefix + "level", action.level);
            cfg.GetString(action_prefix + "message", action.message);
            actions.push_back(action);
        }

        Rule rule;
        rule.id = id;
        rule.category = category;
        rule.enabled = enabled;
        rule.when.sensor_id = sensor_id;
        rule.when.op = op;
        rule.when.value = value;
        rule.then = actions;

        std::vector<Rule> rules = {rule};
        engine.AddRules(rules);

        logger->Info("加载规则: " + id + " (" + category + ")");
    }
}

int GatewayCore::Run(int argc, char* argv[]) {
    // ========== 1. 读配置 ==========
    std::string config_path = "config/environments/development.yaml";

    ConfigManager cfg;
    cfg.LoadYamlFile(config_path);
    CameraApi::SetVideoDevice(cfg.GetStringOr("camera.device", "/dev/video0"));

    // ========== 2. 初始化日志 ==========
    auto sink = std::make_shared<ConsoleSink>();
    auto logger = std::make_shared<Logger>(sink);
    logger->SetLevel(Level::Info);
    logger->Info("网关启动中...");

    // ========== 3. 初始化设备注册表 ==========
    // 注册表存了所有设备的信息（传感器和执行器）
    // HTTP API（/api/devices, /api/status）从这里读数据
    DeviceRegistry registry;
    DeviceApi::SetRegistry(&registry);
    RestApi::SetRegistry(&registry);
    logger->Info("设备注册表已初始化");

    // ========== 4. 加载设备配置 ==========
    // 从 sensors.yaml 和 actuators.yaml 读取设备列表并注册
    std::string topic_prefix = cfg.GetStringOr("mqtt.topic_prefix", "iotgw/dev/");
    LoadDevicesFromConfig(cfg, topic_prefix, registry, logger);

    // ========== 5. 初始化规则引擎 ==========
    // 规则引擎检查传感器值是否超过阈值，超过就触发动作
    RuleEngine rule_engine;
    RuleApi::SetRuleEngine(&rule_engine);
    LoadRulesFromFile("config/rules/automation-rules.yaml", "automation", rule_engine, logger);
    LoadRulesFromFile("config/rules/alarm-rules.yaml", "alarm", rule_engine, logger);
    logger->Info("规则引擎已初始化，共 " + std::to_string(rule_engine.Rules().size()) + " 条规则");

    // ========== 6. 初始化 MQTT 客户端 ==========
    // 这是网关和 ESP32 MQTT 设备之间的通信通道
    // 网关订阅 iotgw/dev/telemetry/# 接收传感器数据
    // 网关发布到 iotgw/dev/cmd/# 发送控制命令
    bool mqtt_enabled = false;
    cfg.GetBool("mqtt.enabled", mqtt_enabled);

    MongooseServer server;
    std::shared_ptr<MqttClient> mqtt_client;

    if (mqtt_enabled) {
        std::string broker_host = cfg.GetStringOr("mqtt.broker_host", "127.0.0.1");
        int64_t broker_port = 1883;
        cfg.GetInt64("mqtt.broker_port", broker_port);
        std::string client_id = cfg.GetStringOr("mqtt.client_id", "iotgw-dev");

        mqtt_client = std::make_shared<MqttClient>(server.GetMgr(), logger);
        DeviceApi::SetMqttClient(mqtt_client.get());
        RestApi::SetMqttClient(mqtt_client.get());

        /**
         * MQTT 消息回调 — 核心通信处理
         *
         * 触发时机：收到 MQTT Broker 转发的消息
         * 消息来源：ESP32 发布的 iotgw/dev/telemetry/{sensor_id}
         *
         * 处理流程：
         *   1. 更新设备注册表（设备在线状态、最新数据）
         *   2. 触发规则引擎（检查传感器值是否超过阈值）
         *      → 超过阈值：发布 MQTT 到 iotgw/dev/cmd/{actuator_id}
         *      → ESP32 MQTT 回调接收
         *      → 转换为 STM32 格式 → 串口发给 STM32
         *   3. WebSocket 广播给所有浏览器客户端
         */
        mqtt_client->SetMessageHandler(
            [&](const std::string& topic, const std::string& payload) {
                // ---- 1. 更新设备注册表 ----
                // UpsertMqttDeviceFromTopic 从 topic 提取设备 ID
                // 如果设备已注册就更新状态，如果没注册就自动创建
                std::string device_id;
                registry.UpsertMqttDeviceFromTopic(topic, payload, NowMs(), device_id);
                logger->Debug("MQTT 消息: " + topic + " → 设备 " + device_id);

                // ---- 2. 触发规则引擎 ----
                // 从 payload 中提取 "value" 字段
                // payload 格式: {"device_id":"temp","type":"sensor","data":{"value":28.4},"ts":...}
                double sensor_value = 0.0;
                bool has_value = false;
                size_t val_pos = payload.find("\"value\"");
                if (val_pos != std::string::npos) {
                    size_t colon = payload.find(':', val_pos);
                    if (colon != std::string::npos) {
                        size_t start = colon + 1;
                        while (start < payload.length() && payload[start] == ' ') start++;
                        size_t end = start;
                        if (end < payload.length() && payload[end] == '-') end++;
                        while (end < payload.length() &&
                               ((payload[end] >= '0' && payload[end] <= '9') || payload[end] == '.')) {
                            end++;
                        }
                        if (end > start) {
                            sensor_value = std::stod(payload.substr(start, end - start));
                            has_value = true;
                        }
                    }
                }

                // 如果提取到了数值，调用规则引擎
                if (has_value && !device_id.empty()) {
                    // OnSensorValue 遍历所有规则，检查条件是否满足
                    // 满足就调用回调函数执行动作
                    rule_engine.OnSensorValue(device_id, sensor_value,
                        [&](const Rule& rule, const Action& action) {
                            if (action.type == "actuator_set") {
                                // 动作类型：控制执行器
                                // 从注册表查执行器的命令主题
                                // 发布 MQTT 到 iotgw/dev/cmd/{actuator_id}
                                // → ESP32 订阅命令主题后接收
                                // → ConvertToStm32Control() 转 STM32 格式
                                // → serial.Write() 写入串口
                                // → STM32 Protocol_ParseAndExecute() 执行
                                std::string cmd_topic;
                                if (registry.GetCommandTopic(action.actuator_id, cmd_topic)) {
                                    mqtt_client->Publish(cmd_topic, action.value);
                                    logger->Info("规则触发: " + rule.id
                                        + " → " + action.actuator_id + "=" + action.value);
                                }
                            } else if (action.type == "log") {
                                // 动作类型：写日志告警
                                if (action.level == "warn") {
                                    logger->Warn("告警规则 " + rule.id + ": " + action.message);
                                } else {
                                    logger->Info("规则 " + rule.id + ": " + action.message);
                                }
                            }
                        });
                }

                // ---- 3. WebSocket 广播 ----
                // 把 MQTT 消息转发给所有连接的浏览器
                // 浏览器收到后更新传感器仪表盘
                std::string frame = "{\"type\":\"mqtt_msg\""
                    ",\"topic\":\"" + topic + "\""
                    ",\"payload\":\"" + payload + "\"}";
                server.BroadcastText(frame);
            }
        );

        // 连接 MQTT Broker
        MqttClient::Options opt;
        opt.url = "mqtt://" + broker_host + ":" + std::to_string(broker_port);
        opt.client_id = client_id;

        if (mqtt_client->Connect(opt)) {
            // 订阅遥测主题：接收 ESP32 发布的传感器数据
            // ESP32 发布到 iotgw/dev/telemetry/temp 等
            // → 这里订阅了 iotgw/dev/telemetry/# 就能收到所有传感器数据
            mqtt_client->Subscribe(topic_prefix + "telemetry/#");
            logger->Info("MQTT 已连接: " + opt.url);
        } else {
            logger->Warn("MQTT 连接失败，继续运行...");
        }
    }

    // ========== 7. 启动 HTTP 服务器 ==========
    server.SetWwwRoot("www");

    // HTTP 请求分发：REST API 或静态文件
    server.SetHttpHandler([](mg_connection* c, mg_http_message* msg) -> bool {
        return RestApi::HandleRequest(c, msg);
    });

    /**
     * WebSocket 消息处理
     *
     * 浏览器通过 WebSocket 发送控制命令，格式:
     *   {"topic":"iotgw/dev/cmd/led","payload":"{\"on\":1,\"br\":50}"}
     *
     * 处理流程：
     *   浏览器 WebSocket → 这里接收 → MQTT 发布到 iotgw/dev/cmd/#
     *   → ESP32 MQTT 回调接收
     *   → ConvertToStm32Control() 转 STM32 格式
     *   → serial.Write() 写入串口
     *   → STM32 Protocol_ParseAndExecute() 控制外设
     */
    server.SetWsHandler([&](mg_connection* c, mg_ws_message* msg) {
        std::string body(msg->data.buf, msg->data.len);

        // 提取 topic 字段
        size_t topic_pos = body.find("\"topic\"");
        if (topic_pos == std::string::npos) {
            mg_ws_printf(c, WEBSOCKET_OP_TEXT,
                "{\"type\":\"error\",\"error\":\"missing_topic\"}");
            return;
        }

        size_t start = body.find('"', topic_pos + 7);
        if (start == std::string::npos) { start = body.find(':', topic_pos) + 1; }
        else { start++; }
        while (start < body.length() && body[start] == ' ') start++;
        size_t end = body.find('"', start);
        std::string topic = body.substr(start, end - start);

        // 提取 payload 值
        std::string payload;
        size_t payload_pos = body.find("\"payload\"");
        if (payload_pos != std::string::npos) {
            size_t p_start = body.find('"', payload_pos + 9);
            if (p_start == std::string::npos) { p_start = body.find(':', payload_pos) + 1; }
            else { p_start++; }
            while (p_start < body.length() && body[p_start] == ' ') p_start++;
            if (body[p_start] == '"') {
                size_t p_end = body.find('"', p_start + 1);
                payload = body.substr(p_start + 1, p_end - p_start - 1);
            } else if (body[p_start] == '{') {
                int depth = 0;
                size_t p_end = p_start;
                for (size_t i = p_start; i < body.length(); i++) {
                    if (body[i] == '{') depth++;
                    if (body[i] == '}') depth--;
                    if (depth == 0) { p_end = i; break; }
                }
                payload = body.substr(p_start, p_end - p_start + 1);
            }
        }

        // 通过 MQTT 发布到 Broker
        // → ESP32 订阅命令主题后接收
        if (mqtt_client && mqtt_client->IsOpen()) {
            mqtt_client->Publish(topic, payload);
            mg_ws_printf(c, WEBSOCKET_OP_TEXT,
                "{\"type\":\"mqtt_pub_ack\",\"ok\":true}");
            logger->Debug("WebSocket → MQTT: " + topic);
        } else {
            mg_ws_printf(c, WEBSOCKET_OP_TEXT,
                "{\"type\":\"error\",\"error\":\"mqtt_not_connected\"}");
        }
    });

    std::string host = cfg.GetStringOr("network.http_api.host", "0.0.0.0");
    int64_t port = 8080;
    cfg.GetInt64("network.http_api.port", port);

    std::string addr = "http://" + host + ":" + std::to_string(port);
    if (!server.Start(addr)) {
        logger->Error("HTTP 服务器启动失败: " + addr + "，请检查端口是否被占用");
        return 1;
    }
    logger->Info("HTTP 服务器已启动: " + addr);

    // ========== 8. 主循环 ==========
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    while (running) {
        server.Poll(50);
    }

    logger->Info("网关正在退出...");
    server.Stop();
    return 0;
}
