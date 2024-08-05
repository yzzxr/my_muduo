#include <memory>

#include "EventLoopThread.h"
#include "EventLoopThreadPool.h"


EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseloop, const std::string& name) :
	_baseloop{ baseloop }, _name{ name }, _started{ false }, _numThreads{ 0 }, _next{ 0 }
{}




void EventLoopThreadPool::start(ThreadInitCallback cb)
{
	_started = true;

	for (int i = 0; i < _numThreads; i++)
	{
		char buf[_name.size() + 32];
		::snprintf(buf, sizeof(buf), "%s%d", _name.data(), i);
		EventLoopThread* t = new EventLoopThread(cb, buf);
		_threads.push_back(std::unique_ptr<EventLoopThread>(t));
		_loops.push_back(t->startLoop()); // 底层创建线程 绑定一个新的EventLoop 并返回该loop的地址
	}

	if (_numThreads == 0 && cb)  // 整个服务端只有一个线程运行baseLoop
		cb(_baseloop);
}


// 如果工作在多线程中，baseLoop_(mainLoop)会默认以轮询的方式分配Channel给subLoop
EventLoop* EventLoopThreadPool::getNextLoop()
{
	// 如果只设置一个线程 也就是只有一个mainReactor 无subReactor 那么轮询只有一个线程 getNextLoop()每次都返回当前的_baseLoop
	EventLoop* loop = _baseloop;

	if (_loops.empty())
	{
		loop = _loops[_next];
		_next++;
		if (_next >= _loops.size())
			_next = 0;
	}

	return loop;
}



std::vector<EventLoop*> EventLoopThreadPool::getAllLoops() const
{
	if (_loops.empty())
		return { _baseloop, };
	else
		return _loops;
}














