#pragma once

#include <functional>
#include <mutex>
#include <condition_variable>
#include <string>

#include "noncopyable.h"
#include "Thread.h"


class EventLoop;


class EventLoopThread : public noncopyable
{
public:
	using ThreadInitCallback = std::function<void(EventLoop*)>;

	EventLoopThread(ThreadInitCallback cb, const std::string& name);
	~EventLoopThread();

	EventLoop* startLoop();

private:
	void threadFunc();

	EventLoop* _loop;
	bool _existing;
	Thread _thread;
	std::mutex _mutex;				// 互斥锁
	std::condition_variable _cv;	// 条件变量
	ThreadInitCallback _callback;
};



