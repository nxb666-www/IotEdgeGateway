#include "zigbee_adapter.hpp"
#include "core/common/logger/logger.hpp"

#include <atomic>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <termios.h>
#include <thread>
#include <unistd.h>

struct ZigbeeAdapter::Impl {
    std::shared_ptr<Logger> logger;
    MessageHandler handler;
    Options options;
    bool connected = false;
    int fd = -1;
    std::thread read_thread;
    std::atomic<bool> running{false};
    std::mutex send_mutex;
    std::string line_buf;

    bool OpenSerial() {
        fd = open(options.serial_port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd < 0) {
            logger->Error("Zigbee open serial failed: " + options.serial_port);
            return false;
        }

        struct termios tty {};
        if (tcgetattr(fd, &tty) != 0) {
            logger->Error("Zigbee get serial attributes failed");
            close(fd);
            fd = -1;
            return false;
        }

        speed_t baud = B115200;
        switch (options.baud_rate) {
            case 9600: baud = B9600; break;
            case 19200: baud = B19200; break;
            case 38400: baud = B38400; break;
            case 57600: baud = B57600; break;
            case 115200: baud = B115200; break;
            default: baud = B115200; break;
        }
        cfsetospeed(&tty, baud);
        cfsetispeed(&tty, baud);

        tty.c_cflag = CS8 | CLOCAL | CREAD;
        tty.c_iflag = 0;
        tty.c_oflag = 0;
        tty.c_lflag = 0;
        tty.c_cc[VMIN] = 0;
        tty.c_cc[VTIME] = 1;

        if (tcsetattr(fd, TCSANOW, &tty) != 0) {
            logger->Error("Zigbee set serial attributes failed");
            close(fd);
            fd = -1;
            return false;
        }

        tcflush(fd, TCIOFLUSH);
        return true;
    }

    void CloseSerial() {
        if (fd >= 0) {
            close(fd);
            fd = -1;
        }
    }

    void ReadLoop() {
        char buf[256];
        while (running) {
            ssize_t n = read(fd, buf, sizeof(buf));
            if (n > 0) {
                line_buf.append(buf, static_cast<size_t>(n));
                size_t pos = 0;
                while ((pos = line_buf.find('\n')) != std::string::npos) {
                    std::string line = line_buf.substr(0, pos);
                    line_buf.erase(0, pos + 1);
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    if (!line.empty() && handler) {
                        logger->Debug("Zigbee rx: " + line);
                        handler("", line);
                    }
                }
            } else {
                usleep(10000);
            }
        }
    }

    bool WriteAll(const std::string& msg) {
        size_t sent = 0;
        while (sent < msg.size()) {
            ssize_t n = write(fd, msg.c_str() + sent, msg.size() - sent);
            if (n < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    usleep(5000);
                    continue;
                }
                return false;
            }
            if (n == 0) {
                usleep(5000);
                continue;
            }
            sent += static_cast<size_t>(n);
        }
        tcdrain(fd);
        return true;
    }
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
        impl_->logger->Info("Zigbee disabled");
        return false;
    }
    if (!impl_->OpenSerial()) return false;
    impl_->running = true;
    impl_->read_thread = std::thread(&Impl::ReadLoop, impl_.get());
    impl_->connected = true;
    impl_->logger->Info("Zigbee connected: " + opts.serial_port + " " + std::to_string(opts.baud_rate));
    return true;
}

void ZigbeeAdapter::Disconnect() {
    impl_->running = false;
    if (impl_->read_thread.joinable()) impl_->read_thread.join();
    impl_->CloseSerial();
    impl_->connected = false;
}

bool ZigbeeAdapter::IsConnected() const {
    return impl_->connected;
}

bool ZigbeeAdapter::SendCommand(const std::string& device_id, const std::string& payload) {
    (void)device_id;
    if (!impl_->connected || impl_->fd < 0) {
        impl_->logger->Warn("Zigbee not connected, cannot send");
        return false;
    }
    std::lock_guard<std::mutex> lock(impl_->send_mutex);
    const std::string msg = payload + "\n";
    for (int repeat = 0; repeat < 2; ++repeat) {
        if (!impl_->WriteAll(msg)) {
            impl_->logger->Error("Zigbee send failed");
            return false;
        }
        if (repeat == 0) usleep(30000);
    }
    impl_->logger->Debug("Zigbee sent: " + payload);
    return true;
}

void ZigbeeAdapter::SetMessageHandler(MessageHandler handler) {
    impl_->handler = std::move(handler);
}
