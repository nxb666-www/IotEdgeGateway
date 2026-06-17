#include "logger.hpp"
#include <iostream>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <sstream>

// Level 转字符串
static const char* LevelToString(Level level) {
    //leve枚举底层本质是数字
    switch(level){
        case Level::Trace: return "Trace";
        case Level::Debug: return "DEBUG";
        case Level::Info:  return "INFO";
        case Level::Warn:  return "WARN";
        case Level::Error: return "ERROR";
        case Level::Fatal: return "FATAL";
        default:           return "UNKNOWN";
    }

}

// 获取当前时间字符串
static std::string NowTimestamp() {
    std::time_t now = std::time(nullptr);
    struct tm* tm=localtime(&now);
    std::ostringstream oss;
    oss<<std::put_time(tm,"%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// ConsoleSink：输出到终端
class ConsoleSink : public Sink {
public:
    void Write(Level level, const std::string& msg) override {
        std::cout<<NowTimestamp()<<"["<<LevelToString(level)<<"]"<<msg<<std::endl;
    }
};

// FileSink：输出到文件
class FileSink : public Sink {
private:
    std::ofstream file_;
public:
    FileSink(const std::string& filename) {
        file_.open(filename,std::ios::app);
    }
    ~FileSink() {
        if(file_.is_open()){
            file_.close();
        }

    }
    void Write(Level level, const std::string& msg) override {
        if(file_.is_open()){
            file_<<NowTimestamp()<<"["<<LevelToString(level)<<"]"<<msg<<std::endl;
        }
    }
};

// Logger 构造函数
Logger::Logger(std::shared_ptr<Sink> sink)
    : level_(Level::Info), sink_(sink) {
}

Logger::~Logger() {
    Flush();
}

void Logger::SetLevel(Level level) {
    level_=level;

}

Level Logger::GetLevel() const {
    return level_;

}

void Logger::Log(Level level, const std::string& msg) {
    if (level < level_) return;              // 1. 过滤
    std::lock_guard<std::mutex> lock(mutex_); // 2. 加锁
    sink_->Write(level, msg);                // 3. 输出
}

void Logger::Trace(const std::string& msg) { Log(Level::Trace, msg); }
void Logger::Debug(const std::string& msg) { Log(Level::Debug, msg); }
void Logger::Info(const std::string& msg)  { Log(Level::Info, msg); }
void Logger::Warn(const std::string& msg)  { Log(Level::Warn, msg); }
void Logger::Error(const std::string& msg) { Log(Level::Error, msg); }
void Logger::Fatal(const std::string& msg) { Log(Level::Fatal, msg); }

void Logger::Flush() {
    std::cout.flush();
}
