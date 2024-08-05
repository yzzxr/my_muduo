#pragma once

#include <functional>
#include <thread>
#include <memory>
#include <unistd.h>
#include <string>
#include <atomic>

#include "noncopyable.h"


/// @brief 封装标准库线程std::thread的线程类
class Thread : public noncopyable
{
public:
	using ThreadFunc = std::function<void()>;

	explicit Thread(ThreadFunc, const std::string& name = std::string());
	~Thread();

	void start();
	void join();

	bool started() { return _started; }
	pid_t tid() const { return _tid; }
	const std::string& name() const { return _name; }

	static int numCreated() { return _numCreated; }

private:
	void setDefaultName();

	bool _started;
	bool _joined;
	std::shared_ptr<std::thread> _thread;	// 封装标准库线程std::thread的线程类
	pid_t _tid;       						// 在线程创建时再绑定
	ThreadFunc _func; 						// 线程回调函数
	std::string _name;
	static std::atomic<int> _numCreated;
};
