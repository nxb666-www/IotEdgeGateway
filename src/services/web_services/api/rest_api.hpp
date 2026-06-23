#ifndef REST_API_HPP
#define REST_API_HPP

#include "mongoose.h"
#include "version.hpp"
#include "device_api.hpp"
#include "rule_api.hpp"
#include "camera_api.hpp"
#include "core/common/utils/json_utils.hpp"
#include <functional>
#include <string>

class SqliteStorage;

// RestApi：路由分发器
// 所有 HTTP 请求从这里进来，根据 URL 路径转给对应的处理函数
// 不干具体活，只负责"URL → 处理函数"的映射
class RestApi {
private:
    static DeviceRegistry* registry_;
    static MqttClient* mqtt_client_;
    static SqliteStorage* sqlite_;
    static std::function<void(const std::string&)> manual_control_callback_;

public:
    static void SetRegistry(DeviceRegistry* r);
    static void SetMqttClient(MqttClient* client);
    static void SetSqliteStorage(SqliteStorage* db);
    static void SetManualControlCallback(std::function<void(const std::string&)> cb);

    // HTTP 请求入口
    // 返回 true = 已处理，false = 不是 API 请求（交给静态文件服务）
    static bool HandleRequest(mg_connection* c, mg_http_message* msg);

private:
    // GET /api/status — 设备状态聚合
    static void HandleStatus(mg_connection* c);

    // POST /api/control — 下发控制指令
    static void HandleControl(mg_connection* c, mg_http_message* msg);

    // GET /api/history?device_id=xxx&limit=100 — 历史遥测数据查询
    static void HandleHistory(mg_connection* c, mg_http_message* msg);
};

#endif
