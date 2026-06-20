#ifndef ADAPTER_BASE_HPP
#define ADAPTER_BASE_HPP

#include <string>

// 协议适配器抽象基类
// 所有协议适配器（MQTT、Zigbee、Modbus）都要实现这三个方法
class AdapterBase {
public:
    virtual ~AdapterBase() = default;
    virtual std::string Name() const = 0;  // 返回适配器名称
    virtual bool Start() = 0;              // 启动
    virtual void Stop() = 0;               // 停止
};

#endif
