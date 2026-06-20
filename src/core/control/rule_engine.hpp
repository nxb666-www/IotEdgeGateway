#ifndef RULE_ENGINE_HPP
#define RULE_ENGINE_HPP

#include <string>
#include <vector>
#include <functional>

// ========== 数据模型 ==========

// 条件：当某个传感器满足某个条件时触发
// 比如：当 temp > 55
struct Condition {
    std::string sensor_id;      // 传感器 ID，比如 "temp"
    std::string op;             // 运算符：>, >=, <, <=, ==, !=
    double value = 0.0;         // 阈值，比如 55.0
};

// 动作：条件满足后执行什么
// 比如：打开风扇，或者写日志报警
struct Action {
    std::string type;           // "actuator_set"（控制执行器）或 "log"（写日志）
    std::string actuator_id;    // 执行器 ID，比如 "motor"（只对 actuator_set 有效）
    std::string value;          // 设置值，比如 "1"（只对 actuator_set 有效）
    std::string level;          // 日志级别，比如 "warn"（只对 log 有效）
    std::string message;        // 日志消息（只对 log 有效）
};

// 规则：一条完整的规则 = 条件 + 动作
// 比如：当 temp > 55 时，打开风扇
struct Rule {
    std::string id;             // 规则 ID，比如 "cooling_on"
    std::string category;       // 分类："automation"（自动化）或 "alarm"（告警）
    bool enabled = true;        // 是否启用
    Condition when;             // 条件
    std::vector<Action> then;   // 动作列表（可以同时执行多个动作）
};

// ========== 规则引擎 ==========

class RuleEngine {
public:
    // 清空所有规则
    void Clear() {
        rules_.clear();
    }

    // 返回所有规则（只读）
    const std::vector<Rule>& Rules() const {
        return rules_;
    }

    // 添加一批规则
    void AddRules(std::vector<Rule> rules) {
        // 把传入的 rules 插入到 rules_ 后面
        rules_.insert(rules_.end(), rules.begin(), rules.end());
    }

    // 启停单条规则
    // 找到 rule_id 对应的规则，设置 enabled
    // 找到返回 true，没找到返回 false
    bool SetEnabled(const std::string& rule_id, bool enabled) {
        // 遍历所有规则
        for (auto& rule : rules_) {
            // 找到 ID 匹配的规则
            if (rule.id == rule_id) {
                rule.enabled = enabled;  // 设置启停
                return true;             // 找到了
            }
        }
        return false;  // 没找到
    }

    // 检查某条规则是否存在
    bool HasRule(const std::string& rule_id) const {
        // 遍历所有规则，看有没有 ID 匹配的
        for (const auto& rule : rules_) {
            if (rule.id == rule_id) return true;
        }
        return false;
    }

    // ========== 核心函数 ==========
    // 传感器值更新时调用这个函数
    // 遍历所有已启用的规则，如果条件满足，就调用 exec 回调
    //
    // 参数：
    //   sensor_id — 传感器 ID，比如 "temp"
    //   value     — 传感器值，比如 25.5
    //   exec      — 回调函数，条件满足时调用，参数是规则和动作
    void OnSensorValue(const std::string& sensor_id, double value,
                       const std::function<void(const Rule& rule,
                                                 const Action& action)>& exec) {
        // 遍历所有规则
        for (const auto& rule : rules_) {
            // 1. 检查是否启用
            if (!rule.enabled) continue;

            // 2. 检查传感器 ID 是否匹配
            if (rule.when.sensor_id != sensor_id) continue;

            // 3. 判断条件是否满足
            if (!Eval(rule.when.op, value, rule.when.value)) continue;

            // 4. 条件满足，遍历动作列表，逐个执行
            for (const auto& action : rule.then) {
                exec(rule, action);  // 调用回调函数执行动作
            }
        }
    }

private:
    std::vector<Rule> rules_;   // 存所有规则

    // 判断条件是否满足
    // op: 运算符（>, >=, <, <=, ==, !=）
    // lhs: 传感器当前值（左边）
    // rhs: 阈值（右边）
    // 返回 true 表示条件满足
    bool Eval(const std::string& op, double lhs, double rhs) {
        if (op == ">")  return lhs > rhs;    // 大于
        if (op == ">=") return lhs >= rhs;   // 大于等于
        if (op == "<")  return lhs < rhs;    // 小于
        if (op == "<=") return lhs <= rhs;   // 小于等于
        if (op == "==") return lhs == rhs;   // 等于
        if (op == "!=") return lhs != rhs;   // 不等于
        return false;                         // 不认识的运算符
    }
};

#endif
