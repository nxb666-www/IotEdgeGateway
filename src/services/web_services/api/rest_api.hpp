#ifndef REST_API_HPP
#define REST_API_HPP

#include "mongoose.h"
#include "version.hpp"
#include "core/device/manager/device_manager.hpp"
#include <string>

class RestApi {
private:
    static DeviceRegistry* registry_;

public:
    static void SetRegistry(DeviceRegistry* reg) {
        registry_ = reg;
    }

    // 处理 HTTP 请求的入口
    static void HandleRequest(mg_connection* c, mg_http_message* msg) {
        struct mg_str caps[2]; // 用于捕获 URL 中的 :id 参数

        // GET /api/health — 健康检查
        if (mg_match(msg->uri, mg_str("/api/health"), NULL)) {
            mg_http_reply(c, 200,
                "Content-Type: application/json\r\n",
                "{\"status\":\"ok\"}\n");

        // GET /api/version — 版本查询
        } else if (mg_match(msg->uri, mg_str("/api/version"), NULL)) {
            mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                "{\"version\":\"%s\"}\n", IOTGW_VERSION);

        // GET /api/devices/:id — 设备详情（必须在 /api/devices 之前匹配）
        } else if (mg_match(msg->uri, mg_str("/api/devices/#"), caps)) {
            std::string device_id(caps[0].ptr, caps[0].len);
            HandleDeviceDetail(c, device_id);

        // GET /api/devices — 设备列表
        } else if (mg_match(msg->uri, mg_str("/api/devices"), NULL)) {
            HandleDeviceList(c);

        // POST /api/actuators/:id/set — 下发命令
        } else if (mg_match(msg->uri, mg_str("/api/actuators/#/set"), caps)) {
            std::string actuator_id(caps[0].ptr, caps[0].len);
            HandleActuatorSet(c, msg, actuator_id);

        // GET /api/status — 设备状态聚合
        } else if (mg_match(msg->uri, mg_str("/api/status"), NULL)) {
            HandleStatus(c);

        // POST /api/control — 下发控制指令
        } else if (mg_match(msg->uri, mg_str("/api/control"), NULL)) {
            HandleControl(c, msg);

        // 都不匹配，返回 404
        } else {
            mg_http_reply(c, 404, "Content-Type: application/json\r\n",
                "{\"error\":\"not_found\"}\n");
        }
    }

private:
    // GET /api/devices — 设备列表
    static void HandleDeviceList(mg_connection* c) {
        if (!registry_) {
            mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                "{\"error\":\"registry not initialized\"}\n");
            return;
        }
        std::string json = registry_->ToJsonList();
        mg_http_reply(c, 200, "Content-Type: application/json\r\n",
            "%s", json.c_str());
    }

    // GET /api/devices/:id — 设备详情
    static void HandleDeviceDetail(mg_connection* c, const std::string& id) {
        if (!registry_) {
            mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                "{\"error\":\"registry not initialized\"}\n");
            return;
        }
        std::string json;
        if (registry_->ToJsonOne(id, json)) {
            mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                "%s", json.c_str());
        } else {
            mg_http_reply(c, 404, "Content-Type: application/json\r\n",
                "{\"error\":\"device_not_found\"}\n");
        }
    }

    // POST /api/actuators/:id/set — 下发命令
    static void HandleActuatorSet(mg_connection* c, mg_http_message* msg,
                                   const std::string& actuator_id) {
        // 获取请求体
        std::string body(msg->body.ptr, msg->body.len);

        // TODO: 通过 MQTT 发布命令到执行器
        // 暂时只返回 ok
        mg_http_reply(c, 200, "Content-Type: application/json\r\n",
            "{\"ok\":true}\n");
    }

    // GET /api/status — 设备状态聚合
    static void HandleStatus(mg_connection* c) {
        if (!registry_) {
            mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                "{\"error\":\"registry not initialized\"}\n");
            return;
        }

        // 扫描所有设备，解析 last_payload 中的字段
        auto devices = registry_->List();
        std::string json = "{";
        bool first = true;
        for (const auto& dev : devices) {
            if (!first) json += ",";
            first = false;
            // 简单输出设备ID和最后的payload
            json += "\"" + dev.id + "\":" + dev.status.last_payload;
        }
        json += "}\n";

        mg_http_reply(c, 200, "Content-Type: application/json\r\n",
            "%s", json.c_str());
    }

    // POST /api/control — 下发控制指令
    static void HandleControl(mg_connection* c, mg_http_message* msg) {
        // TODO: 解析请求体，通过 MQTT 下发给对应执行器
        mg_http_reply(c, 200, "Content-Type: application/json\r\n",
            "{\"status\":\"ok\"}\n");
    }
};

// 静态成员初始化
DeviceRegistry* RestApi::registry_ = nullptr;

#endif
