#ifndef SQLITE_STORAGE_HPP
#define SQLITE_STORAGE_HPP

#include <string>
#include <cstdint>

// 前置声明，不暴露 sqlite3.h 给其他模块
struct sqlite3;

/**
 * SqliteStorage — 传感器历史数据持久化
 *
 * 职责：
 *   1. 打开/创建 SQLite 数据库
 *   2. 建表（telemetry_history）
 *   3. 插入传感器遥测数据
 *   4. 查询历史数据（返回 JSON）
 *   5. 清理过期数据
 *
 * 用法：
 *   SqliteStorage storage;
 *   storage.Open("data/iotgw.db");
 *   storage.InitSchema();
 *   storage.InsertTelemetry("temp", "iotgw/dev/telemetry/temp", payload, NowMs());
 *   std::string json = storage.QueryHistoryJson("temp", 100, 0, 0);
 *   storage.Close();
 */
class SqliteStorage {
public:
    SqliteStorage();
    ~SqliteStorage();

    // 打开数据库，db_path 不存在会自动创建
    bool Open(const std::string& db_path);

    // 关闭数据库
    void Close();

    // 建表（如果不存在）
    bool InitSchema();

    /**
     * 插入一条遥测数据
     *
     * @param device_id   设备 ID，如 "temp"
     * @param topic       MQTT 主题，如 "iotgw/dev/telemetry/temp"
     * @param payload     原始 MQTT payload JSON
     * @param created_at  网关收到数据时的 Unix 毫秒时间戳
     * @return true 成功，false 失败
     */
    bool InsertTelemetry(const std::string& device_id,
                         const std::string& topic,
                         const std::string& payload,
                         int64_t created_at);

    /**
     * 查询历史数据，返回 JSON 字符串
     *
     * @param device_id  设备 ID
     * @param limit      返回条数，默认 100，最大 1000
     * @param from_ms    起始时间戳（0=不限）
     * @param to_ms      结束时间戳（0=不限）
     * @return JSON 字符串，如 {"device_id":"temp","items":[...]}
     */
    std::string QueryHistoryJson(const std::string& device_id,
                                 int limit,
                                 int64_t from_ms,
                                 int64_t to_ms);

    // 清理 N 天前的数据
    bool CleanupOlderThan(int retention_days);

    // 数据库是否已打开
    bool IsOpen() const;

private:
    sqlite3* db_;
};

#endif
