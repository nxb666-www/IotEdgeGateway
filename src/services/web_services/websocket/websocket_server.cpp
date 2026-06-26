#include "websocket_server.hpp"
#include "services/web_services/api/camera_api.hpp"

#include <cstring>
#include <utility>

// Mongoose event handler (friend of MongooseServer)
void on_event(struct mg_connection *c, int ev, void *ev_data) {
    auto *srv = static_cast<MongooseServer *>(c->fn_data);
    if (!srv && c->mgr) {
        srv = static_cast<MongooseServer *>(c->mgr->userdata);
        c->fn_data = srv;
    }
    if (!srv) return;

    if (ev == MG_EV_HTTP_MSG) {
        auto *hm = static_cast<mg_http_message *>(ev_data);

        // WebSocket upgrade
        if (mg_match(hm->uri, mg_str("/ws"), NULL)) {
            mg_ws_upgrade(c, hm, NULL);
            return;
        }

        // Try custom HTTP handler first
        if (srv->http_handler_ && srv->http_handler_(c, hm)) {
            return;
        }

        // Fallback to static file serving
        struct mg_http_serve_opts opts = {};
        opts.root_dir = srv->www_root_.c_str();
        mg_http_serve_dir(c, hm, &opts);
        return;
    }

    if (ev == MG_EV_WS_MSG) {
        auto *wm = static_cast<mg_ws_message *>(ev_data);
        if (srv->ws_handler_) {
            srv->ws_handler_(c, wm);
        }
        return;
    }

    // MJPEG: send next frame on every poll tick
    if (ev == MG_EV_POLL) {
        if (CameraApi::IsMjpegConnection(c)) {
            CameraApi::HandleMjpegPoll(c);
        }
        return;
    }
}

MongooseServer::MongooseServer()
    : running_(false), listener_(nullptr) {
    mg_mgr_init(&mgr_);
    mgr_.userdata = this;
}

MongooseServer::~MongooseServer() {
    Stop();
}

bool MongooseServer::Start(const std::string &addr) {
    mgr_.userdata = this;

    listener_ = mg_http_listen(&mgr_, addr.c_str(), on_event, this);
    if (!listener_) {
        return false;
    }

    // Point every connection's fn_data back to this server so on_event can find it
    listener_->fn_data = this;
    running_ = true;
    return true;
}

void MongooseServer::Poll(int timeout_ms) {
    mg_mgr_poll(&mgr_, timeout_ms);
    // mg_mgr_poll may create new connections; ensure fn_data is set
    for (struct mg_connection *nc = mgr_.conns; nc != NULL; nc = nc->next) {
        if (nc->fn_data == nullptr) {
            nc->fn_data = this;
        }
    }
}

void MongooseServer::Stop() {
    if (running_) {
        mg_mgr_free(&mgr_);
        running_ = false;
    }
}

void MongooseServer::SetHttpHandler(HttpHandler handler) {
    http_handler_ = std::move(handler);
}

void MongooseServer::SetWsHandler(WsHandler handler) {
    ws_handler_ = std::move(handler);
}

void MongooseServer::SetWwwRoot(const std::string &path) {
    www_root_ = path;
}

void MongooseServer::BroadcastText(const std::string &msg) {
    for (struct mg_connection *c = mgr_.conns; c != NULL; c = c->next) {
        if (c->is_websocket) {
            mg_ws_send(c, msg.c_str(), msg.size(), WEBSOCKET_OP_TEXT);
        }
    }
}
