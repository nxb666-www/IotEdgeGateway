#ifndef RULE_API_HPP
#define RULE_API_HPP

#include "mongoose.h"
#include "core/control/rule_engine.hpp"
#include <string>

// RuleApi：规则 API 处理器
// 前端通过这些接口查看、启停规则
class RuleApi {
private:
    static RuleEngine* engine_;     // 规则引擎指针
    static bool reload_flag_;       // 重新加载标志

public:
    // 注入规则引擎指针
    static void SetRuleEngine(RuleEngine* engine);

    // GET /api/rules — 返回所有规则列表
    static void HandleRuleList(mg_connection* c);

    // POST /api/rules/reload — 重新加载规则配置文件
    static void HandleRuleReload(mg_connection* c);

    // POST /api/rules/:id/enable — 启用规则
    static void HandleRuleEnable(mg_connection* c, const std::string& rule_id);

    // POST /api/rules/:id/disable — 禁用规则
    static void HandleRuleDisable(mg_connection* c, const std::string& rule_id);

    // 检查是否需要重新加载规则
    static bool NeedReload();
};

#endif
