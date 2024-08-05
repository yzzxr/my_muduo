#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>

#include "noncopyable.h"

class EventLoop;
class EventLoopThread;


class EventLoopThreadPool : public noncopyable
{
public:
	using ThreadInitCallback = std::function<void(EventLoop*)>;

	EventLoopThreadPool(EventLoop* baseloop, const std::string& name);
	~EventLoopThreadPool();

	//设置线程数量
	void setThreadNum(int n) { _numThreads = n; }
	//启动线程池
	void start(ThreadInitCallback cb);

	// 工作在多线程中，_baseLoop(mainLoop)会默认以轮询的方式分配Channel给subLoop
	EventLoop* getNextLoop();

	std::vector<EventLoop*> getAllLoops() const;

	bool started() const { return _started; }
	const std::string& name() const { return _name; }

private:
	EventLoop* _baseloop; // 用户使用muduo创建的loop,如果线程数为1那直接使用用户创建的loop,否则创建多EventLoop
	std::string _name;
	bool _started;
	int _numThreads;
	int _next;  // 轮询的下标
	std::vector<std::unique_ptr<EventLoopThread>> _threads;
	std::vector<EventLoop*> _loops;
};
