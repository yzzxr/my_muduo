#include <iostream>

#include "Logger.h"
#include "Timestamp.h"

// 获取日志唯一的实例对象 单例
Logger& Logger::instance()
{
	static Logger logger;
	return logger;
}

// 设置日志级别
void Logger::setLogLevel(LogLevel level)
{
	_logLevel = level;
}

// 写日志 [级别信息] time : msg
void Logger::log(std::string msg)
{
	std::string pre;
	switch (_logLevel)
	{
	case LogLevel::INFO:
		pre.append("[INFO]");
		break;
	case LogLevel::ERROR:
		pre.append("[ERROR]");
		break;
	case LogLevel::FATAL:
		pre.append("[FATAL]");
		break;
	case LogLevel::DEBUG:
		pre.append("[DEBUG]");
		break;
	}
	// 打印时间和msg
	std::cout << pre.append(Timestamp::now().toString()) << " : " << msg;
}