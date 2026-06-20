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

// 全局变量，控制程序退出
static bool running = true;

// Ctrl+C 信号处理
static void signal_handler(int sig) {
    running = false;
}

// 获取当前时间戳（毫秒）
static int64_t NowMs() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

// 从配置文件加载设备并注册
static void LoadDevicesFromConfig(ConfigManager& cfg,
                                   const std::string& topic_prefix,
                                   DeviceRegistry& registry,
                                   std::shared_ptr<Logger> logger) {
    // 读取 sensors.yaml
    ConfigManager sensors_cfg;
    if (sensors_cfg.LoadYamlFile("config/devices/sensors.yaml")) {
        // 遍历 sensors 列表
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

    // 读取 actuators.yaml
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

// 从 YAML 文件加载规则
// file_path: 规则文件路径，比如 "config/rules/automation-rules.yaml"
// category: 规则分类，"automation" 或 "alarm"
// engine: 规则引擎指针
static void LoadRulesFromFile(const std::string& file_path,
                               const std::string& category,
                               RuleEngine& engine,
                               std::shared_ptr<Logger> logger) {
    ConfigManager cfg;
    if (!cfg.LoadYamlFile(file_path)) {
        logger->Warn("规则文件加载失败: " + file_path);
        return;
    }

    // 遍历规则列表（YAML 里的 key 是 automation_rules 或 alarm_rules）
    std::string key = category + "_rules";
    for (int i = 0; ; i++) {
        std::string prefix = key + "[" + std::to_string(i) + "].";

        // 读取规则 ID
        std::string id;
        if (!cfg.GetString(prefix + "id", id)) break;

        // 读取是否启用
        bool enabled = true;
        cfg.GetBool(prefix + "enabled", enabled);

        // 读取条件
        std::string sensor_id, op;
        double value = 0.0;
        cfg.GetString(prefix + "when.sensor_id", sensor_id);
        cfg.GetString(prefix + "when.op", op);
        // 阈值可能是 int 或 double，先试 GetString 再转
        std::string value_str;
        if (cfg.GetString(prefix + "when.value", value_str)) {
            value = std::stod(value_str);
        }

        // 读取动作列表
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

        // 构造 Rule 结构体
        Rule rule;
        rule.id = id;
        rule.category = category;
        rule.enabled = enabled;
        rule.when.sensor_id = sensor_id;
        rule.when.op = op;
        rule.when.value = value;
        rule.then = actions;

        // 加入规则引擎
        std::vector<Rule> rules = {rule};
        engine.AddRules(rules);

        logger->Info("加载规则: " + id + " (" + category + ")");
    }
}

int GatewayCore::Run(int argc, char* argv[]) {
    // 1. 解析命令行参数
    std::string config_path = "config/environments/development.yaml";
    // TODO: 解析 --yaml-config 和 --log-level

    // 2. 读配置
    ConfigManager cfg;
    cfg.LoadYamlFile(config_path);

    // 3. 初始化日志
    auto sink = std::make_shared<ConsoleSink>();
    auto logger = std::make_shared<Logger>(sink);
    logger->SetLevel(Level::Info);
    logger->Info("网关启动中...");

    // 4. 初始化设备注册表
    DeviceRegistry registry;
    DeviceApi::SetRegistry(&registry);
    RestApi::SetRegistry(&registry);
    logger->Info("设备注册表已初始化");

    // 5. 加载设备配置
    std::string topic_prefix = cfg.GetStringOr("mqtt.topic_prefix", "iotgw/dev/");
    LoadDevicesFromConfig(cfg, topic_prefix, registry, logger);

    // 5.5 初始化规则引擎
    RuleEngine rule_engine;
    RuleApi::SetRuleEngine(&rule_engine);
    // 加载规则配置文件
    LoadRulesFromFile("config/rules/automation-rules.yaml", "automation", rule_engine, logger);
    LoadRulesFromFile("config/rules/alarm-rules.yaml", "alarm", rule_engine, logger);
    logger->Info("规则引擎已初始化，共 " + std::to_string(rule_engine.Rules().size()) + " 条规则");

    // 6. 初始化 MQTT 客户端
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

        mqtt_client->SetMessageHandler(
            [&](const std::string& topic, const std::string& payload) {
                // 1. 更新设备注册表
                std::string device_id;
                registry.UpsertMqttDeviceFromTopic(topic, payload, NowMs(), device_id);
                logger->Debug("MQTT 消息: " + topic + " → 设备 " + device_id);

                // 2. 触发规则引擎
                // 从 payload 中提取 "value" 字段
                double sensor_value = 0.0;
                bool has_value = false;
                size_t val_pos = payload.find("\"value\"");
                if (val_pos != std::string::npos) {
                    size_t colon = payload.find(':', val_pos);
                    if (colon != std::string::npos) {
                        // 跳过空格，读取数字
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

                // 如果提取到了数值，触发规则引擎
                if (has_value && !device_id.empty()) {
                    rule_engine.OnSensorValue(device_id, sensor_value,
                        [&](const Rule& rule, const Action& action) {
                            if (action.type == "actuator_set") {
                                // 查注册表找执行器的命令主题
                                std::string cmd_topic;
                                if (registry.GetCommandTopic(action.actuator_id, cmd_topic)) {
                                    mqtt_client->Publish(cmd_topic, action.value);
                                    logger->Info("规则触发: " + rule.id
                                        + " → " + action.actuator_id + "=" + action.value);
                                }
                            } else if (action.type == "log") {
                                // 写日志
                                if (action.level == "warn") {
                                    logger->Warn("告警规则 " + rule.id + ": " + action.message);
                                } else {
                                    logger->Info("规则 " + rule.id + ": " + action.message);
                                }
                            }
                        });
                }

                // 3. 通过 WebSocket 广播给所有浏览器客户端
                std::string frame = "{\"type\":\"mqtt_msg\""
                    ",\"topic\":\"" + topic + "\""
                    ",\"payload\":\"" + payload + "\"}";
                server.BroadcastText(frame);
            }
        );

        MqttClient::Options opt;
        opt.url = "mqtt://" + broker_host + ":" + std::to_string(broker_port);
        opt.client_id = client_id;

        if (mqtt_client->Connect(opt)) {
            mqtt_client->Subscribe(topic_prefix + "telemetry/#");
            logger->Info("MQTT 已连接: " + opt.url);
        } else {
            logger->Warn("MQTT 连接失败，继续运行...");
        }
    }

    // 7. 启动 HTTP 服务器
    server.SetWwwRoot("www");

    server.SetHttpHandler([](mg_connection* c, mg_http_message* msg) -> bool {
        return RestApi::HandleRequest(c, msg);
    });

    // WebSocket 消息处理
    // 浏览器通过 WebSocket 发消息给网关，格式: {"topic":"...","payload":"..."}
    // 网关通过 MQTT 发布到 Broker
    server.SetWsHandler([&](mg_connection* c, mg_ws_message* msg) {
        std::string body(msg->data.buf, msg->data.len);

        // 提取 topic 字段
        size_t topic_pos = body.find("\"topic\"");
        if (topic_pos == std::string::npos) {
            // 没有 topic，返回错误
            mg_ws_printf(c, WEBSOCKET_OP_TEXT,
                "{\"type\":\"error\",\"error\":\"missing_topic\"}");
            return;
        }

        // 提取 topic 值
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
            // payload 可能是字符串 "..." 或对象 {...}
            if (body[p_start] == '"') {
                // 字符串格式
                size_t p_end = body.find('"', p_start + 1);
                payload = body.substr(p_start + 1, p_end - p_start - 1);
            } else if (body[p_start] == '{') {
                // 对象格式，找到匹配的 }
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

        // 通过 MQTT 发布
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
    server.Start(addr);
    logger->Info("HTTP 服务器已启动: " + addr);

    // 8. 注册信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 9. 事件循环
    while (running) {
        server.Poll(50);
    }

    // 10. 退出
    logger->Info("网关正在退出...");
    server.Stop();
    return 0;
}
