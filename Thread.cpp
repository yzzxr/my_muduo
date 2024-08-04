#include <semaphore.h>

#include "Thread.h"
#include "CurrentThread.h"


std::atomic<int> Thread::_numCreated(0);

Thread::Thread(ThreadFunc func, const std::string& name)
	: _started(false)
	, _joined(false)
	, _tid(0)
	, _func(std::move(func))
	, _name(name)
{
	setDefaultName();
}

Thread::~Thread()
{
	if (_started && !_joined)
	{
		_thread->detach();                                                  // thread类提供了设置分离线程的方法 线程运行后自动销毁（非阻塞）
	}
}

void Thread::start()                                                        // 一个Thread对象 记录的就是一个新线程的详细信息
{
	_started = true;
	sem_t sem;
	sem_init(&sem, false, 0);                                               // false指的是 不设置进程间共享
	// 开启线程
	_thread = std::shared_ptr<std::thread>(new std::thread([&]() {
		_tid = CurrentThread::tid();                                        // 获取线程的tid值
		sem_post(&sem);
		_func();                                                            // 开启一个新线程 专门执行该线程函数
	}));

	// 这里必须等待获取上面新创建的线程的tid值
	sem_wait(&sem);
}

// C++ std::thread 中join()和detach()的区别：https://blog.nowcoder.net/n/8fcd9bb6e2e94d9596cf0a45c8e5858a
void Thread::join()
{
	_joined = true;
	_thread->join();
}

void Thread::setDefaultName()
{
	int num = ++_numCreated;
	if (_name.empty())
	{
		char buf[32] = { 0 };
		snprintf(buf, sizeof(buf), "Thread%d", num);
		_name = buf;
	}
}


