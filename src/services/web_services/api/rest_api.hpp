#ifndef REST_API_HPP
#define REST_API_HPP

#include "mongoose.h"
#include "version.hpp"
#include "device_api.hpp"
#include "rule_api.hpp"
#include <string>

// RestApi：路由分发器
// 所有 HTTP 请求从这里进来，根据 URL 路径转给对应的处理函数
// 不干具体活，只负责"URL → 处理函数"的映射
//
// 类比：快递分拣中心
//   快递（HTTP请求）进来 → 看地址（URL） → 送到对应的部门（处理函数）
class RestApi {
private:
    // 依赖的外部对象（通过 Set 方法注入）
    static DeviceRegistry* registry_;   // 设备注册表
    static MqttClient* mqtt_client_;    // MQTT 客户端

public:
    // 注入设备注册表（启动时调用一次）
    static void SetRegistry(DeviceRegistry* r);

    // 注入 MQTT 客户端（启动时调用一次）
    static void SetMqttClient(MqttClient* client);

    // HTTP 请求入口
    // 所有 HTTP 请求都从这里进来
    // 根据 URL 匹配路由，调用对应的处理函数
    // 返回 true = 已处理，false = 不是 API 请求（交给静态文件服务）
    static bool HandleRequest(mg_connection* c, mg_http_message* msg);

private:
    // GET /api/status — 设备状态聚合
    // 遍历所有设备，取每个设备的最新数据，合并成一个 JSON 返回
    // 前端每 1 秒轮询这个接口，显示传感器数据
    static void HandleStatus(mg_connection* c);

    // POST /api/control — 下发控制指令
    // 前端控制面板发 LED/电机/蜂鸣器命令到这里
    // 解析 JSON 后通过 MQTT 下发给对应的执行器
    static void HandleControl(mg_connection* c, mg_http_message* msg);

    // JSON 工具函数
    // 从 JSON 中提取 value 字段（支持简单格式和信封格式）
    static std::string ExtractValue(const std::string& json);
    // 从 JSON 中指定位置提取数字
    static std::string ExtractNumber(const std::string& json, size_t key_pos);
    // 从 JSON 中提取整数值
    static int GetJsonInt(const std::string& json, const std::string& key);
};

#endif
