#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <memory>
#include <mutex>

/**
 * 日志级别枚举
 * 从低到高：Trace < Debug < Info < Warn < Error < Fatal
 * 设置某个级别后，只有 >= 该级别的日志才会输出
 */
enum class Level {
    Trace,  // 最详细，追踪程序每一步
    Debug,  // 调试信息
    Info,   // 普通信息，比如"服务器启动"
    Warn,   // 警告，不影响运行但要注意
    Error,  // 错误，某个功能失败了
    Fatal   // 致命错误，程序要挂了
};

/**
 * Sink 接口：决定日志输出到哪里
 * 子类实现 Write 方法：
 *   - ConsoleSink：输出到终端
 *   - FileSink：输出到文件
 */
class Sink {
public:
    virtual ~Sink() = default;
    virtual void Write(Level level, const std::string& msg) = 0;
};

/**
 * Logger 类：日志系统的核心
 * 用法：
 *   auto sink = std::make_shared<ConsoleSink>();
 *   auto logger = std::make_shared<Logger>(sink);
 *   logger->Info("服务器启动");
 *   logger->Error("连接失败");
 */
class Logger {
private:
    Level level_;                       // 当前日志级别，低于此级别的日志不输出
    std::shared_ptr<Sink> sink_;        // 日志输出目标（终端/文件）
    std::mutex mutex_;                  // 互斥锁，保证多线程安全

public:
 // 构造函数，传入输出目标
    Logger(std::shared_ptr<Sink> sink);
    ~Logger();

    void SetLevel(Level level);         // 设置日志级别
    Level GetLevel() const;             // 获取当前日志级别

    /**
     * Log：核心日志函数
     * 第一：直接输出消息
     * 第二：带标签，格式为 "[tag] message"
     */
    void Log(Level level, const std::string& msg);


    // 便捷方法，不用每次写 Log(Level::Info, ...)
    void Trace(const std::string& msg); // 最详细日志
    void Debug(const std::string& msg); // 调试信息
    void Info(const std::string& msg);  // 普通信息
    void Warn(const std::string& msg);  // 警告
    void Error(const std::string& msg); // 错误
    void Fatal(const std::string& msg); // 致命错误

    void Flush();                       // 刷新日志缓冲区
};

#endif
