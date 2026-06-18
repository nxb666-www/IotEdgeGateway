#ifndef WEBSOCKET_SERVER_HPP
#define WEBSOCKET_SERVER_HPP

#include <string>
#include <functional>
#include "mongoose.h"

class MongooseServer {
    // 声明 on_event 为友元函数，这样它能访问 private 成员
    friend void on_event(struct mg_connection*, int, void*);
public:
    // 回调类型：HTTP 请求来了调这个
    using HttpHandler = std::function<void(struct mg_connection*, struct mg_http_message*)>;
    // 回调类型：WebSocket 消息来了调这个
    using WsHandler = std::function<void(struct mg_connection*, struct mg_ws_message*)>;

private:
    struct mg_mgr mgr_;            // Mongoose 管理器
    bool running_;                 // 是否运行中
    std::string www_root_;         // 静态文件目录
    HttpHandler http_handler_;     // HTTP 回调
    WsHandler ws_handler_;         // WebSocket 回调

public:
    MongooseServer();
    ~MongooseServer();

    bool Start(const std::string& addr);   // 启动服务器，监听地址
    void Poll(int timeout_ms);             // 轮询事件
    void Stop();                           // 停止服务器

    void SetHttpHandler(HttpHandler handler);   // 设置 HTTP 回调
    void SetWsHandler(WsHandler handler);       // 设置 WebSocket 回调
    void SetWwwRoot(const std::string& path);   // 设置静态文件目录

    void BroadcastText(const std::string& msg); // WebSocket 广播
};

#endif
