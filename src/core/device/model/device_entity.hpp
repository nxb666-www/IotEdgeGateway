#ifndef DEVICE_ENTITY_HPP
#define DEVICE_ENTITY_HPP

#include "device_status.hpp"
#include <string>

// 设备实体：描述一个设备的完整信息
struct DeviceEntity {
    std::string id;                // 设备ID，比如 "temp"、"led"
    std::string kind;              // 设备类型: "sensor" | "actuator" | "unknown"
    std::string transport;         // 通信方式: "mqtt" | "zigbee" | "modbus" | "uart"
    std::string telemetry_topic;   // 遥测数据主题，比如 "iotgw/dev/telemetry/temp"
    std::string command_topic;     // 命令下发主题，比如 "iotgw/dev/cmd/led"
    DeviceStatus status;           // 设备状态
};

#endif
