#include "zigbee_adapter.hpp"
#include "core/common/logger/logger.hpp"

// Zigbee 适配器占位实现
// 当前仅提供框架和日志，不实际连接硬件
// 后续接入 Zigbee 协调器时在此实现串口通信

struct ZigbeeAdapter::Impl {
    std::shared_ptr<Logger> logger;
    MessageHandler handler;
    Options options;
    bool connected = false;
};

ZigbeeAdapter::ZigbeeAdapter(std::shared_ptr<Logger> logger)
    : impl_(std::make_unique<Impl>()) {
    impl_->logger = logger;
}

ZigbeeAdapter::~ZigbeeAdapter() {
    Disconnect();
}

bool ZigbeeAdapter::Connect(const Options& opts) {
    impl_->options = opts;

    if (!opts.enabled) {
        impl_->logger->Info("Zigbee 适配器: 未启用（配置 zigbee.enabled=false）");
        return false;
    }

    // 占位：当前不实际连接硬件
    impl_->logger->Warn("Zigbee 适配器: 当前为占位实现，未接入实际硬件");
    impl_->logger->Info("Zigbee 适配器: 串口=" + opts.serial_port
                        + " 波特率=" + std::to_string(opts.baud_rate));
    impl_->connected = false;
    return false;
}

void ZigbeeAdapter::Disconnect() {
    if (impl_->connected) {
        impl_->logger->Info("Zigbee 适配器: 断开连接");
        impl_->connected = false;
    }
}

bool ZigbeeAdapter::IsConnected() const {
    return impl_->connected;
}

bool ZigbeeAdapter::SendCommand(const std::string& device_id, const std::string& payload) {
    if (!impl_->connected) {
        impl_->logger->Warn("Zigbee 适配器: 未连接，无法发送命令到 " + device_id);
        return false;
    }
    // 占位：实际实现时通过串口发送 Zigbee 帧
    impl_->logger->Debug("Zigbee 适配器: 发送命令 → " + device_id + " " + payload);
    return true;
}

void ZigbeeAdapter::SetMessageHandler(MessageHandler handler) {
    impl_->handler = std::move(handler);
}
