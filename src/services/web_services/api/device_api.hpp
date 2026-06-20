#ifndef DEVICE_API_HPP
#define DEVICE_API_HPP

#include "mongoose.h"
#include "core/device/manager/device_manager.hpp"
#include "core/device/protocol_adapters/mqtt_adapter/mqtt_adapter.hpp"
#include <string>

class DeviceApi {
private:
    static DeviceRegistry* registry_;
    static MqttClient* mqtt_client_;

public:
    static void SetRegistry(DeviceRegistry* reg);
    static void SetMqttClient(MqttClient* client);
    static DeviceRegistry* GetRegistry();

    // GET /api/devices — 设备列表
    static void HandleDeviceList(mg_connection* c);

    // GET /api/devices/:id — 设备详情
    static void HandleDeviceDetail(mg_connection* c, const std::string& id);

    // POST /api/actuators/:id/set — 下发命令
    static void HandleActuatorSet(mg_connection* c, mg_http_message* msg,
                                   const std::string& actuator_id);
};

#endif
