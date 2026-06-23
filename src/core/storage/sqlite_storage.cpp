#include "sqlite_storage.hpp"
#include "core/common/logger/logger.hpp"
#include "core/common/utils/json_utils.hpp"
#include "sqlite3.h"

#include <sstream>
#include <vector>

SqliteStorage::SqliteStorage() : db_(nullptr) {}

SqliteStorage::~SqliteStorage() {
    Close();
}

bool SqliteStorage::Open(const std::string& db_path) {
    Close();

    // sqlite3_open 会自动创建不存在的数据库文件
    int rc = sqlite3_open(db_path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        // sqlite3_open 失败时 db_ 可能非空，需要关闭
        if (db_) {
            sqlite3_close(db_);
            db_ = nullptr;
        }
        return false;
    }

    // 开启 WAL 模式，提高并发读写性能
    sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    // 设置忙等待超时（毫秒）
    sqlite3_busy_timeout(db_, 5000);

    return true;
}

void SqliteStorage::Close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool SqliteStorage::IsOpen() const {
    return db_ != nullptr;
}

bool SqliteStorage::InitSchema() {
    if (!db_) return false;

    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS telemetry_history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            device_id TEXT NOT NULL,
            type TEXT NOT NULL,
            source TEXT,
            driver TEXT,
            value REAL,
            raw_payload TEXT NOT NULL,
            topic TEXT,
            ts INTEGER,
            created_at INTEGER NOT NULL
        );

        CREATE INDEX IF NOT EXISTS idx_telemetry_device_time
        ON telemetry_history(device_id, created_at);
    )";

    char* err = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::string error = err ? err : "unknown error";
        sqlite3_free(err);
        return false;
    }

    return true;
}

bool SqliteStorage::InsertTelemetry(const std::string& device_id,
                                     const std::string& topic,
                                     const std::string& payload,
                                     int64_t created_at) {
    if (!db_) return false;

    // 从 payload 中解析字段
    std::string type, source, driver;
    double value = 0.0;
    int64_t ts = 0;

    // 用 json_utils 提取字段
    type = json_utils::ExtractValue(payload);
    // 如果 type 为空说明解析失败，设默认值
    if (type.empty()) type = "sensor";

    // 手动解析各字段
    // 解析 "type"
    size_t pos = payload.find("\"type\"");
    if (pos != std::string::npos) {
        size_t colon = payload.find(':', pos);
        if (colon != std::string::npos) {
            size_t start = payload.find('"', colon + 1);
            if (start != std::string::npos) {
                size_t end = payload.find('"', start + 1);
                if (end != std::string::npos) {
                    type = payload.substr(start + 1, end - start - 1);
                }
            }
        }
    }

    // 解析 "source"
    pos = payload.find("\"source\"");
    if (pos != std::string::npos) {
        size_t colon = payload.find(':', pos);
        if (colon != std::string::npos) {
            size_t start = payload.find('"', colon + 1);
            if (start != std::string::npos) {
                size_t end = payload.find('"', start + 1);
                if (end != std::string::npos) {
                    source = payload.substr(start + 1, end - start - 1);
                }
            }
        }
    }

    // 解析 "driver"
    pos = payload.find("\"driver\"");
    if (pos != std::string::npos) {
        size_t colon = payload.find(':', pos);
        if (colon != std::string::npos) {
            size_t start = payload.find('"', colon + 1);
            if (start != std::string::npos) {
                size_t end = payload.find('"', start + 1);
                if (end != std::string::npos) {
                    driver = payload.substr(start + 1, end - start - 1);
                }
            }
        }
    }

    // 解析 "value"（从 data.value）
    pos = payload.find("\"value\"");
    if (pos != std::string::npos) {
        size_t colon = payload.find(':', pos);
        if (colon != std::string::npos) {
            size_t start = colon + 1;
            while (start < payload.length() && payload[start] == ' ') start++;
            size_t end = start;
            if (end < payload.length() && payload[end] == '-') end++;
            while (end < payload.length() &&
                   ((payload[end] >= '0' && payload[end] <= '9') || payload[end] == '.')) {
                end++;
            }
            if (end > start) {
                value = std::stod(payload.substr(start, end - start));
            }
        }
    }

    // 解析 "ts"
    pos = payload.find("\"ts\"");
    if (pos != std::string::npos) {
        size_t colon = payload.find(':', pos);
        if (colon != std::string::npos) {
            size_t start = colon + 1;
            while (start < payload.length() && payload[start] == ' ') start++;
            size_t end = start;
            while (end < payload.length() && payload[end] >= '0' && payload[end] <= '9') {
                end++;
            }
            if (end > start) {
                ts = std::stoll(payload.substr(start, end - start));
            }
        }
    }

    // 预编译 SQL 语句，防止 SQL 注入
    const char* sql = R"(
        INSERT INTO telemetry_history
        (device_id, type, source, driver, value, raw_payload, topic, ts, created_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return false;
    }

    // 绑定参数（索引从 1 开始）
    sqlite3_bind_text(stmt, 1, device_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, source.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, driver.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 5, value);
    sqlite3_bind_text(stmt, 6, payload.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, topic.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 8, ts);
    sqlite3_bind_int64(stmt, 9, created_at);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

std::string SqliteStorage::QueryHistoryJson(const std::string& device_id,
                                              int limit,
                                              int64_t from_ms,
                                              int64_t to_ms) {
    if (!db_) return "{\"error\":\"数据库未打开\"}";

    // 限制返回条数
    if (limit <= 0) limit = 100;
    if (limit > 1000) limit = 1000;

    // 构造 SQL
    std::string sql = "SELECT value, ts, created_at FROM telemetry_history "
                      "WHERE device_id = ?";
    if (from_ms > 0) sql += " AND created_at >= ?";
    if (to_ms > 0) sql += " AND created_at <= ?";
    sql += " ORDER BY created_at DESC LIMIT ?";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return "{\"error\":\"查询失败\"}";
    }

    // 绑定参数
    int idx = 1;
    sqlite3_bind_text(stmt, idx++, device_id.c_str(), -1, SQLITE_TRANSIENT);
    if (from_ms > 0) sqlite3_bind_int64(stmt, idx++, from_ms);
    if (to_ms > 0) sqlite3_bind_int64(stmt, idx++, to_ms);
    sqlite3_bind_int(stmt, idx++, limit);

    // 构造 JSON 结果
    std::string json = "{\"device_id\":\"" + json_utils::Escape(device_id) + "\",\"items\":[";

    bool first = true;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) json += ",";
        first = false;

        double value = sqlite3_column_double(stmt, 0);
        int64_t ts = sqlite3_column_int64(stmt, 1);
        int64_t created_at = sqlite3_column_int64(stmt, 2);

        json += "{\"value\":";
        // 保留一位小数
        std::ostringstream oss;
        oss << value;
        json += oss.str();
        json += ",\"ts\":" + std::to_string(ts);
        json += ",\"created_at\":" + std::to_string(created_at);
        json += "}";
    }

    json += "]}";

    sqlite3_finalize(stmt);
    return json;
}

bool SqliteStorage::CleanupOlderThan(int retention_days) {
    if (!db_) return false;
    if (retention_days <= 0) return false;

    // 计算截止时间戳（当前时间 - 保留天数）
    int64_t cutoff_ms = 0;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    cutoff_ms = (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    cutoff_ms -= (int64_t)retention_days * 24 * 60 * 60 * 1000;

    std::string sql = "DELETE FROM telemetry_history WHERE created_at < " + std::to_string(cutoff_ms);

    char* err = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::string error = err ? err : "unknown error";
        sqlite3_free(err);
        return false;
    }

    // 获取删除的行数
    int changes = sqlite3_changes(db_);
    return true;
}
