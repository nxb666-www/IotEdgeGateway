#include "gateway_core.hpp"
#include "core/common/config/config_manager.hpp"
#include "core/common/logger/logger.hpp"
#include "core/device/manager/device_manager.hpp"
#include "services/web_services/websocket/websocket_server.hpp"
#include "services/web_services/api/rest_api.hpp"
#include "mongoose.h"
#include <csignal>

// 全局变量，控制程序退出
static bool running = true;

// Ctrl+C 信号处理
static void signal_handler(int sig) {
    running = false;
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
    RestApi::SetRegistry(&registry);
    logger->Info("设备注册表已初始化");

    // 5. 启动服务器
    MongooseServer server;
    server.SetWwwRoot("www");

    // 6. 设置 HTTP 回调
    server.SetHttpHandler([](mg_connection* c, mg_http_message* msg) {
        RestApi::HandleRequest(c, msg);
    });

    // 7. 从配置读端口
    std::string host = cfg.GetStringOr("network.http_api.host", "0.0.0.0");
    int64_t port = 8080;
    cfg.GetInt64("network.http_api.port", port);

    std::string addr = "http://" + host + ":" + std::to_string(port);
    server.Start(addr);
    logger->Info("服务器已启动: " + addr);

    // 7. 注册信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 8. 事件循环
    while (running) {
        server.Poll(50);
    }

    // 9. 退出
    logger->Info("网关正在退出...");
    server.Stop();
    return 0;
}
