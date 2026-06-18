#include "websocket_server.hpp"
// 回调函数：Mongoose 有事件就调这个
void on_event(mg_connection* c, int ev, void* ev_data) {
    MongooseServer* server = (MongooseServer*)c->fn_data;

    if (ev == MG_EV_HTTP_MSG) {
        // HTTP 请求来了，调设置的回调
        if (server->http_handler_) {
            server->http_handler_(c, (mg_http_message*)ev_data);
        }
    } else if (ev == MG_EV_WS_MSG) {
        // WebSocket 消息来了，调设置的回调
        if (server->ws_handler_) {
            server->ws_handler_(c, (mg_ws_message*)ev_data);
        }
    }
}

MongooseServer::MongooseServer()
:running_(false)
{
    mg_mgr_init(&mgr_);
}
MongooseServer::~MongooseServer(){
    Stop();
}
// 启动服务器，监听地址
bool MongooseServer::Start(const std::string& addr){
    mg_http_listen(&mgr_,addr.c_str(),on_event,this);
    running_ = true;
    return true;
}  
// 轮询事件
void MongooseServer::Poll(int timeout_ms){
     mg_mgr_poll(&mgr_, timeout_ms);
}             
// 停止服务器
void MongooseServer::Stop(){
    running_ = false;
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