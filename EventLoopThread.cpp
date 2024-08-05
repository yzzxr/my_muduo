#include "EventLoopThread.h"
#include "Thread.h"
#include "EventLoop.h"


EventLoopThread::EventLoopThread(ThreadInitCallback cb, const std::string& name) : 
		_loop{nullptr}, _existing{false}, _thread(std::bind(EventLoopThread::threadFunc, this), name), 
		_callback{std::move(cb)}
{

}


EventLoopThread::~EventLoopThread()
{
	_existing = true;
	if (_loop != nullptr)
		_loop->quit(), _thread.join();
}



EventLoop* EventLoopThread::startLoop()
{
	_thread.start(); // 启用底层线程Thread类对象_thread中通过start()创建的线程

	EventLoop* loop = nullptr;
	{
		std::unique_lock<std::mutex> guard(_mutex);
		while (_loop == nullptr)
			_cv.wait(guard);
		loop = _loop;
	}
	return loop;
}


// 下面这个方法 是在单独的新线程里运行的
void EventLoopThread::threadFunc()
{
	EventLoop loop;  // 创建一个独立的EventLoop对象 和上面的线程是一一对应的 one loop per thread

	if (_callback)
		_callback(&loop);
	
	{
		std::unique_lock<std::mutex> lock(_mutex);
		_loop = &loop;
		_cv.notify_one();
	}

	loop.loop();

	{
		std::unique_lock<std::mutex> lock(_mutex);
		_loop = nullptr;
	}
}







