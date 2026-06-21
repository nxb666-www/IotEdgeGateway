#include "websocket_server.hpp"
// 回调函数：Mongoose 有事件就调这个
void on_event(mg_connection* c, int ev, void* ev_data) {
    MongooseServer* server = (MongooseServer*)c->fn_data;

    if (ev == MG_EV_HTTP_MSG) {
        // HTTP 请求来了，调设置的回调
        mg_http_message* hm = (mg_http_message*)ev_data;
        bool handled = false;
        if (server->http_handler_) {
            handled = server->http_handler_(c, hm);
        }
        // 如果 API 没处理（非 /api/* 请求），回退到静态文件服务
        if (!handled) {
            struct mg_http_serve_opts opts;
            memset(&opts, 0, sizeof(opts));
            opts.root_dir = server->www_root_.c_str();
            mg_http_serve_dir(c, hm, &opts);
        }
    } else if (ev == MG_EV_WS_MSG) {
        // WebSocket 消息来了，调设置的回调
        if (server->ws_handler_) {
            server->ws_handler_(c, (mg_ws_message*)ev_data);
        }
    }
}

MongooseServer::MongooseServer()
:running_(false), listener_(nullptr)
{
    mg_mgr_init(&mgr_);
}
MongooseServer::~MongooseServer(){
    Stop();
}
// 启动服务器，监听地址
bool MongooseServer::Start(const std::string& addr){
    listener_ = mg_http_listen(&mgr_,addr.c_str(),on_event,this);
    if (listener_ == nullptr) {
        running_ = false;
        return false;
    }
    running_ = true;
    return true;
}  
// 轮询事件
void MongooseServer::Poll(int timeout_ms){
     mg_mgr_poll(&mgr_, timeout_ms);
}             
// 停止服务器
void MongooseServer::Stop(){
    if (!running_ && listener_ == nullptr) return;
    running_ = false;
    listener_ = nullptr;
    mg_mgr_free(&mgr_);
}            
// 设置 HTTP 回调
void MongooseServer::SetHttpHandler(HttpHandler handler){
    http_handler_ = handler;
}   
// 设置 WebSocket 回调
void MongooseServer::SetWsHandler(WsHandler handler){
    ws_handler_ = handler;
}   
// 设置静态文件目录   
void MongooseServer::SetWwwRoot(const std::string& path){
    www_root_ = path;
}  
// WebSocket 广播：给所有连接发消息
void MongooseServer::BroadcastText(const std::string& msg) {
    struct mg_connection* c;
    for (c = mgr_.conns; c != NULL; c = c->next) {
        if (c->is_websocket) {
            mg_ws_send(c, msg.c_str(), msg.size(), WEBSOCKET_OP_TEXT);
        }
    }
}
