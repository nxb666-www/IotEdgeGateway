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
#include "core/common/utils/json_utils.hpp"
#include "core/common/logger/logger.hpp"
#include "core/device/manager/device_manager.hpp"
#include "core/device/protocol_adapters/mqtt_adapter/mqtt_adapter.hpp"
#include "services/web_services/websocket/websocket_server.hpp"
#include "services/web_services/api/rest_api.hpp"
#include "services/web_services/api/device_api.hpp"
#include "services/web_services/api/rule_api.hpp"
#include "core/control/rule_engine.hpp"
#include "core/storage/sqlite_storage.hpp"
#include "mongoose.h"
#include <atomic>
#include <chrono>
#include <csignal>
#include <ctime>
#include <thread>
#include <unordered_map>
#include <sys/stat.h>
#include <sys/types.h>

static std::atomic_bool running(true);

static void signal_handler(int sig) {
    running = false;
}

static int64_t NowMs() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static Level ParseLogLevel(const std::string& value) {
    if (value == "trace") return Level::Trace;
    if (value == "debug") return Level::Debug;
    if (value == "warn") return Level::Warn;
    if (value == "error") return Level::Error;
    if (value == "fatal") return Level::Fatal;
    return Level::Info;
}

static void EnsureParentDir(const std::string& file_path) {
    size_t pos = 0;
    while ((pos = file_path.find('/', pos)) != std::string::npos) {
        std::string dir = file_path.substr(0, pos);
        if (!dir.empty()) {
            mkdir(dir.c_str(), 0755);
        }
        pos++;
    }
}

static std::string ArgValue(int argc, char* argv[], const std::string& name,
                            const std::string& default_value) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (argv[i] && name == argv[i]) {
            return argv[i + 1] ? argv[i + 1] : default_value;
        }
    }
    return default_value;
}

static bool JsonHasField(const std::string& json, const std::string& key) {
    return json.find("\"" + key + "\"") != std::string::npos;
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
    std::string config_path = ArgValue(argc, argv, "--yaml-config",
        "config/environments/development.yaml");

    ConfigManager cfg;
    cfg.LoadYamlFile(config_path);
    CameraApi::SetVideoDevice(cfg.GetStringOr("camera.device", "/dev/video0"));

    // ========== 2. 初始化日志 ==========
    auto sink = std::make_shared<MultiSink>();
    sink->Add(std::make_shared<ConsoleSink>());
    std::string log_file = ArgValue(argc, argv, "--log-file",
        cfg.GetStringOr("paths.log_file", ""));
    if (!log_file.empty()) {
        EnsureParentDir(log_file);
        sink->Add(std::make_shared<FileSink>(log_file));
    }
    auto logger = std::make_shared<Logger>(sink);
    logger->SetLevel(ParseLogLevel(ArgValue(argc, argv, "--log-level",
        cfg.GetStringOr("logging.level", "info"))));
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
    // 手动控制优先级：记录每个执行器的手动控制截止时间
    std::unordered_map<std::string, int64_t> manual_control_until_ms;
    int64_t manual_override_ms = 30000;
    cfg.GetInt64("control.manual_override_ms", manual_override_ms);
    // 规则引擎去重：记录每个执行器上次发布的命令，避免重复发送淹没串口
    std::unordered_map<std::string, std::string> last_rule_cmd;
    RestApi::SetManualControlCallback([&](const std::string& payload) {
        int64_t until = NowMs() + manual_override_ms;
        if (JsonHasField(payload, "led_on")) {
            manual_control_until_ms["led"] = until;
            last_rule_cmd.erase("led");
            logger->Info("manual control priority: led");
        }
        if (JsonHasField(payload, "motor_on")) {
            manual_control_until_ms["motor"] = until;
            last_rule_cmd.erase("motor");
            logger->Info("manual control priority: motor");
        }
        if (JsonHasField(payload, "buzzer")) {
            manual_control_until_ms["buzzer"] = until;
            last_rule_cmd.erase("buzzer");
            logger->Info("manual control priority: buzzer");
        }
    });
    logger->Info("规则引擎已初始化，共 " + std::to_string(rule_engine.Rules().size()) + " 条规则");

    // ========== 5.5 初始化 SQLite 存储 ==========
    SqliteStorage sqlite;
    bool storage_enabled = false;
    cfg.GetBool("storage.enabled", storage_enabled);
    if (storage_enabled) {
        std::string db_path = cfg.GetStringOr("storage.db_path", "data/iotgw.db");
        int64_t retention_days = 7;
        cfg.GetInt64("storage.retention_days", retention_days);
        EnsureParentDir(db_path);
        if (sqlite.Open(db_path) && sqlite.InitSchema()) {
            RestApi::SetSqliteStorage(&sqlite);
            sqlite.CleanupOlderThan((int)retention_days);
            logger->Info("SQLite 存储已启用: " + db_path
                         + " (保留 " + std::to_string(retention_days) + " 天)");
        } else {
            logger->Warn("SQLite 初始化失败，继续运行...");
        }
    }

    // ========== 6. 初始化 MQTT 客户端 ==========
    // 这是网关和 ESP32 MQTT 设备之间的通信通道
    // 网关订阅 iotgw/dev/telemetry/# 接收传感器数据
    // 网关发布到 iotgw/dev/cmd/# 发送控制命令
    bool mqtt_enabled = false;
    cfg.GetBool("mqtt.enabled", mqtt_enabled);

    MongooseServer server;
    std::shared_ptr<MqttClient> mqtt_client;
    MqttClient::Options mqtt_options;
    std::string mqtt_telemetry_topic;
    int64_t last_mqtt_reconnect_ms = 0;

    if (mqtt_enabled) {
        std::string broker_host = cfg.GetStringOr("mqtt.broker_host", "127.0.0.1");
        int64_t broker_port = 1883;
        cfg.GetInt64("mqtt.broker_port", broker_port);
        std::string client_id = cfg.GetStringOr("mqtt.client_id", "iotgw-dev");
        int64_t keepalive_sec = mqtt_options.keepalive_sec;
        cfg.GetInt64("mqtt.keepalive_sec", keepalive_sec);
        bool clean_session = mqtt_options.clean_session;
        cfg.GetBool("mqtt.clean_session", clean_session);
        mqtt_options.url = "mqtt://" + broker_host + ":" + std::to_string(broker_port);
        mqtt_options.client_id = client_id;
        mqtt_options.keepalive_sec = static_cast<uint16_t>(keepalive_sec <= 0 ? 30 : keepalive_sec);
        mqtt_options.clean_session = clean_session;
        mqtt_telemetry_topic = topic_prefix + "telemetry/#";

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

                // ---- 1.5 SQLite 保存历史数据 ----
                // 只保存遥测主题，不保存命令主题
                std::string telemetry_prefix = topic_prefix + "telemetry/";
                if (sqlite.IsOpen() && topic.substr(0, telemetry_prefix.length()) == telemetry_prefix) {
                    sqlite.InsertTelemetry(device_id, topic, payload, NowMs());
                }

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
                                std::string cmd_topic;
                                if (registry.GetCommandTopic(action.actuator_id, cmd_topic)) {
                                    // 手动控制优先：在手动覆盖窗口内跳过规则
                                    auto manual_it = manual_control_until_ms.find(action.actuator_id);
                                    if (manual_it != manual_control_until_ms.end() && NowMs() < manual_it->second) {
                                        logger->Debug("rule skipped by manual priority: "
                                            + rule.id + " -> " + action.actuator_id);
                                        return;
                                    }
                                    // 命令去重：和上次发布的命令相同则跳过，避免淹没串口
                                    auto last_it = last_rule_cmd.find(action.actuator_id);
                                    if (last_it != last_rule_cmd.end() && last_it->second == action.value) {
                                        return;
                                    }
                                    last_rule_cmd[action.actuator_id] = action.value;
                                    mqtt_client->Publish(cmd_topic, action.value);
                                    logger->Info("规则触发: " + rule.id
                                        + " → " + action.actuator_id + "=" + action.value);
                                }
                            } else if (action.type == "log") {
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
                    ",\"topic\":" + json_utils::Quote(topic) +
                    ",\"payload\":" + json_utils::Quote(payload) + "}";
                server.BroadcastText(frame);
            }
        );

        // 连接 MQTT Broker
        if (mqtt_client->Connect(mqtt_options)) {
            // 订阅遥测主题：接收 ESP32 发布的传感器数据
            // ESP32 发布到 iotgw/dev/telemetry/temp 等
            // → 这里订阅了 iotgw/dev/telemetry/# 就能收到所有传感器数据
            mqtt_client->Subscribe(mqtt_telemetry_topic);
            logger->Info("MQTT 已连接: " + mqtt_options.url);
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
    int64_t port = 8081;
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

    std::thread event_loop([&]() {
        while (running) {
            server.Poll(50);
        }
    });

    while (running) {
        if (mqtt_enabled && mqtt_client && !mqtt_client->IsOpen() &&
            NowMs() - last_mqtt_reconnect_ms >= 3000) {
            last_mqtt_reconnect_ms = NowMs();
            logger->Warn("MQTT 已断开，尝试重连: " + mqtt_options.url);
            if (mqtt_client->Connect(mqtt_options)) {
                mqtt_client->Subscribe(mqtt_telemetry_topic);
                logger->Info("MQTT 已重连并重新订阅: " + mqtt_telemetry_topic);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    if (event_loop.joinable()) {
        event_loop.join();
    }

    logger->Info("网关正在退出...");
    server.Stop();
    return 0;
}
