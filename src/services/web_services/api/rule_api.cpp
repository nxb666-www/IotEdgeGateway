#include "rule_api.hpp"

// 静态成员初始化
RuleEngine* RuleApi::engine_ = nullptr;
bool RuleApi::reload_flag_ = false;

// 注入规则引擎指针
void RuleApi::SetRuleEngine(RuleEngine* engine) {
    engine_ = engine;
}

// GET /api/rules — 返回所有规则列表
void RuleApi::HandleRuleList(mg_connection* c) {
    if (!engine_) {
        mg_http_reply(c, 500, "Content-Type: application/json\r\n",
            "{\"error\":\"规则引擎未初始化\"}\n");
        return;
    }

    // 遍历所有规则，拼成 JSON 数组
    const auto& rules = engine_->Rules();
    std::string json = "[";
    bool first = true;

    for (const auto& rule : rules) {
        if (!first) json += ",";
        first = false;

        json += "{";
        json += "\"id\":\"" + rule.id + "\"";
        json += ",\"category\":\"" + rule.category + "\"";
        json += ",\"enabled\":" + std::string(rule.enabled ? "true" : "false");
        json += ",\"sensor_id\":\"" + rule.when.sensor_id + "\"";
        json += ",\"op\":\"" + rule.when.op + "\"";
        json += ",\"value\":" + std::to_string(rule.when.value);
        json += "}";
    }

    json += "]\n";
    mg_http_reply(c, 200, "Content-Type: application/json\r\n",
        "%s", json.c_str());
}

// POST /api/rules/reload — 设置标志，通知 GatewayCore 重新加载规则
void RuleApi::HandleRuleReload(mg_connection* c) {
    reload_flag_ = true;
    mg_http_reply(c, 200, "Content-Type: application/json\r\n",
        "{\"ok\":true}\n");
}

// POST /api/rules/:id/enable — 启用规则
void RuleApi::HandleRuleEnable(mg_connection* c, const std::string& rule_id) {
    if (!engine_) {
        mg_http_reply(c, 500, "Content-Type: application/json\r\n",
            "{\"error\":\"规则引擎未初始化\"}\n");
        return;
    }

    if (engine_->SetEnabled(rule_id, true)) {
        mg_http_reply(c, 200, "Content-Type: application/json\r\n",
            "{\"ok\":true}\n");
    } else {
        mg_http_reply(c, 404, "Content-Type: application/json\r\n",
            "{\"ok\":false}\n");
    }
}

// POST /api/rules/:id/disable — 禁用规则
void RuleApi::HandleRuleDisable(mg_connection* c, const std::string& rule_id) {
    if (!engine_) {
        mg_http_reply(c, 500, "Content-Type: application/json\r\n",
            "{\"error\":\"规则引擎未初始化\"}\n");
        return;
    }

    if (engine_->SetEnabled(rule_id, false)) {
        mg_http_reply(c, 200, "Content-Type: application/json\r\n",
            "{\"ok\":true}\n");
    } else {
        mg_http_reply(c, 404, "Content-Type: application/json\r\n",
            "{\"ok\":false}\n");
    }
}

// 检查是否需要重新加载规则
bool RuleApi::NeedReload() {
    if (reload_flag_) {
        reload_flag_ = false;
        return true;
    }
    return false;
}
