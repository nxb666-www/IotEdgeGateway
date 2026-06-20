#include "mqtt_adapter.hpp"

// MQTT 事件回调函数
// Mongoose 收到 MQTT 事件时调用这个函数
void mqtt_event_handler(struct mg_connection* c, int ev, void* ev_data) {
    // 从连接取出 MqttClient 对象指针
    MqttClient* client = (MqttClient*)c->fn_data;

    if (ev == MG_EV_MQTT_OPEN) {
        // MQTT 连接成功
    } else if (ev == MG_EV_MQTT_MSG) {
        // 收到 MQTT 消息
        struct mg_mqtt_message* msg = (struct mg_mqtt_message*)ev_data;

        // 提取主题和数据（Mongoose 7.15 用 buf 不是 ptr）
        std::string topic(msg->topic.buf, msg->topic.len);
        std::string payload(msg->data.buf, msg->data.len);

        // 调用用户设置的消息处理回调
        if (client->message_handler_) {
            client->message_handler_(topic, payload);
        }
    } else if (ev == MG_EV_ERROR) {
        // 连接出错
    }
}

// 构造函数：保存 Mongoose 管理器和日志器
MqttClient::MqttClient(struct mg_mgr* mgr, std::shared_ptr<Logger> logger)
    : mgr_(mgr), logger_(logger), conn_(nullptr), connected_(false) {
}

// 连接 MQTT Broker
bool MqttClient::Connect(const Options& opt) {
    if (!mgr_) return false;

    // 构建连接选项（字段名要和 Mongoose 7.15 对应）
    struct mg_mqtt_opts opts;
    memset(&opts, 0, sizeof(opts));
    opts.client_id = mg_str(opt.client_id.c_str());
    opts.keepalive = opt.keepalive_sec;      // Mongoose 7.15 用 keepalive
    opts.clean = opt.clean_session;          // Mongoose 7.15 用 clean

    // 如果有用户名密码
    if (!opt.user.empty()) {
        opts.user = mg_str(opt.user.c_str());
    }
    if (!opt.pass.empty()) {
        opts.pass = mg_str(opt.pass.c_str());
    }

    // 发起 MQTT 连接
    conn_ = mg_mqtt_connect(mgr_, opt.url.c_str(), &opts,
                            mqtt_event_handler, this);

    if (conn_ == nullptr) {
        if (logger_) logger_->Error("MQTT 连接失败: " + opt.url);
        return false;
    }

    // 保存选项，重连时用
    options_ = opt;
    connected_ = true;

    if (logger_) logger_->Info("MQTT 连接中: " + opt.url);
    return true;
}

// 订阅主题
bool MqttClient::Subscribe(const std::string& topic, uint8_t qos) {
    if (!conn_ || !connected_) {
        if (logger_) logger_->Warn("MQTT 未连接，无法订阅");
        return false;
    }

    // Mongoose 7.15 的 mg_mqtt_sub 需要 mg_mqtt_opts 结构体
    struct mg_mqtt_opts opts;
    memset(&opts, 0, sizeof(opts));
    opts.topic = mg_str(topic.c_str());
    opts.qos = qos;
    mg_mqtt_sub(conn_, &opts);

    if (logger_) logger_->Info("MQTT 订阅: " + topic);
    return true;
}

// 发布消息
bool MqttClient::Publish(const std::string& topic, const std::string& payload,
                          uint8_t qos, bool retain) {
    if (!conn_ || !connected_) {
        if (logger_) logger_->Warn("MQTT 未连接，无法发布");
        return false;
    }

    // 构建发布选项
    struct mg_mqtt_opts opts;
    memset(&opts, 0, sizeof(opts));
    opts.topic = mg_str(topic.c_str());
    opts.message = mg_str(payload.c_str());
    opts.qos = qos;
    opts.retain = retain;

    mg_mqtt_pub(conn_, &opts);

    if (logger_) logger_->Debug("MQTT 发布: " + topic + " " + payload);
    return true;
}

// 设置消息处理回调
void MqttClient::SetMessageHandler(MessageHandler handler) {
    message_handler_ = handler;
}

// 检查连接是否正常
bool MqttClient::IsOpen() const {
    return conn_ != nullptr && connected_;
}
