#ifndef DEVICE_MANAGER_HPP
#define DEVICE_MANAGER_HPP

#include "core/device/model/device_entity.hpp"
#include <map>
#include <string>
#include <mutex>
#include <vector>

// 设备注册表：管理所有设备
// 内部维护三个映射表：
//   by_id_                — 设备ID → 设备实体
//   telemetry_topic_to_id_ — 遥测主题 → 设备ID（收到遥测消息时快速找到设备）
//   command_topic_to_id_   — 命令主题 → 设备ID（下发命令时快速找到设备）
class DeviceRegistry {
private:
    std::map<std::string, DeviceEntity> by_id_;
    std::map<std::string, std::string> telemetry_topic_to_id_;
    std::map<std::string, std::string> command_topic_to_id_;
    mutable std::mutex mutex_;

public:
    // 注册设备
    bool Register(DeviceEntity device);

    // 设备是否存在
    bool Has(const std::string& id) const;

    // 获取设备信息
    bool Get(const std::string& id, DeviceEntity& out) const;

    // 返回所有设备列表
    std::vector<DeviceEntity> List() const;

    // 通过遥测主题更新设备状态
    // 找到设备后更新 online、last_seen_ms、last_payload、last_topic
    // out_device_id 返回找到的设备ID
    bool UpdateFromTelemetryTopic(const std::string& topic,
                                  const std::string& payload,
                                  int64_t now_ms,
                                  std::string& out_device_id);

    // 从 MQTT 消息自动注册/更新设备
    // 如果设备未注册，自动从主题提取ID并创建
    bool UpsertMqttDeviceFromTopic(const std::string& topic,
                                   const std::string& payload,
                                   int64_t now_ms,
                                   std::string& out_device_id);

    // 获取设备的命令主题
    bool GetCommandTopic(const std::string& device_id, std::string& out_topic) const;

    // 获取设备的遥测主题
    bool GetTelemetryTopic(const std::string& device_id, std::string& out_topic) const;

    // 返回 JSON 数组字符串（所有设备）
    std::string ToJsonList() const;

    // 返回单个设备的 JSON 字符串
    bool ToJsonOne(const std::string& id, std::string& out_json) const;
};

#endif
