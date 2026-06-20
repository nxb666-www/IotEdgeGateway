#include "device_api.hpp"

// 静态成员初始化
// 静态变量属于类，不属于对象，必须在类外初始化
DeviceRegistry* DeviceApi::registry_ = nullptr;
MqttClient* DeviceApi::mqtt_client_ = nullptr;

// 保存注册表指针
// 在 gateway_core.cpp 启动时调用，把注册表传进来
void DeviceApi::SetRegistry(DeviceRegistry* reg) {
    registry_ = reg;
}

// 保存 MQTT 客户端指针
// 在 gateway_core.cpp 启动时调用，把 MQTT 客户端传进来
void DeviceApi::SetMqttClient(MqttClient* client) {
    mqtt_client_ = client;
}

// 返回注册表指针
// 给 rest_api.hpp 的 HandleStatus 用
DeviceRegistry* DeviceApi::GetRegistry() {
    return registry_;
}

// GET /api/devices — 设备列表
// 浏览器访问 /api/devices 时执行这个函数
// 从注册表取出所有设备，转成 JSON 数组返回
void DeviceApi::HandleDeviceList(mg_connection* c) {
    // 先检查注册表有没有初始化
    if (!registry_) {
        mg_http_reply(c, 500, "Content-Type: application/json\r\n",
            "{\"error\":\"设备注册表未初始化\"}\n");
        return;
    }
    // 调注册表的 ToJsonList() 转成 JSON 字符串
    std::string json = registry_->ToJsonList();
    // 返回 200 状态码 + JSON 数据
    mg_http_reply(c, 200, "Content-Type: application/json\r\n",
        "%s", json.c_str());
}

// GET /api/devices/:id — 设备详情
// 浏览器访问 /api/devices/temp 时执行
// 从注册表取出指定设备，转成 JSON 返回
void DeviceApi::HandleDeviceDetail(mg_connection* c, const std::string& id) {
    if (!registry_) {
        mg_http_reply(c, 500, "Content-Type: application/json\r\n",
            "{\"error\":\"设备注册表未初始化\"}\n");
        return;
    }
    std::string json;
    // 调注册表的 ToJsonOne() 查找设备
    if (registry_->ToJsonOne(id, json)) {
        // 找到了，返回设备信息
        mg_http_reply(c, 200, "Content-Type: application/json\r\n",
            "%s", json.c_str());
    } else {
        // 没找到，返回 404
        mg_http_reply(c, 404, "Content-Type: application/json\r\n",
            "{\"error\":\"设备未找到\"}\n");
    }
}

// POST /api/actuators/:id/set — 下发命令
// 浏览器发 POST 请求到 /api/actuators/led/set 时执行
// 请求体是 {"value":1}，通过 MQTT 发给执行器
void DeviceApi::HandleActuatorSet(mg_connection* c, mg_http_message* msg,
                                   const std::string& actuator_id) {
    // 先检查 MQTT 是否连接
    // 没连接就返回 503（服务不可用）
    if (!mqtt_client_ || !mqtt_client_->IsOpen()) {
        mg_http_reply(c, 503, "Content-Type: application/json\r\n",
            "{\"ok\":false,\"error\":\"MQTT未连接\"}\n");
        return;
    }

    // 取出请求体（浏览器发来的 JSON）
    std::string body(msg->body.buf, msg->body.len);

    // 从注册表获取执行器的命令主题
    // 比如 led 的命令主题是 "iotgw/dev/cmd/led"
    std::string cmd_topic;
    if (registry_ && registry_->GetCommandTopic(actuator_id, cmd_topic)) {
        // 通过 MQTT 发布命令到 Broker
        // Broker 会把命令转发给执行器
        mqtt_client_->Publish(cmd_topic, body);
    }

    // 返回成功
    mg_http_reply(c, 200, "Content-Type: application/json\r\n",
        "{\"ok\":true,\"message\":\"命令已发送\"}\n");
}
