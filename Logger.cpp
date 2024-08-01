#include <iostream>

#include "Logger.h"
#include "Timestamp.h"

// 获取日志唯一的实例对象 单例
Logger &Logger::instance()
{
    static Logger logger;
    return logger;
}

// 设置日志级别
void Logger::setLogLevel(LogLevel level)
{
    logLevel_ = level;
}

// 写日志 [级别信息] time : msg
void Logger::log(std::string msg)
{
    std::string pre;
    switch (logLevel_)
    {
        case LogLevel::INFO:
            pre = "[INFO]";
            break;
        case LogLevel::ERROR:
            pre = "[ERROR]";
            break;
        case LogLevel::FATAL:
            pre = "[FATAL]";
            break;
        case LogLevel::DEBUG:
            pre = "[DEBUG]";
            break;
        default:
            break;
    }

    // 打印时间和msg
    std::cout << pre.append(Timestamp::now().toString()) << " : " << msg << std::endl;
}