#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <memory>


#include "EventLoop.h"
#include "Logger.h"
#include "Channel.h"
#include "Poller.h"


/// @brief 防止一个线程创建多个EventLoop
thread_local EventLoop* loopInThisThread = nullptr;

/// @brief 默认的Poller IO复用接口的超时时间
constexpr int PollTimeout = 10000;


/* 
 * 创建线程之后主线程和子线程谁先运行是不确定的。
 * 通过一个eventfd在线程之间传递数据的好处是多个线程无需上锁就可以实现同步。
 * eventfd支持的最低内核版本为Linux 2.6.27,在2.6.26及之前的版本也可以使用eventfd，但是flags必须设置为0。
 * 函数原型：
 *     #include <sys/eventfd.h>
 *     int eventfd(unsigned int initval, int flags);
 * 参数说明：
 *      initval,初始化计数器的值。
 *      flags, EFD_NONBLOCK,设置socket为非阻塞。
 *             EFD_CLOEXEC，执行fork的时候，在父进程中的描述符会自动关闭，子进程中的描述符保留。
 * 场景：
 *     eventfd可以用于同一个进程之中的线程之间的通信。
 *     eventfd还可以用于同亲缘关系的进程之间的通信。
 *     eventfd用于不同亲缘关系的进程之间通信的话需要把eventfd放在几个进程共享的共享内存中（没有测试过）。
*/
inline int createEventfd()   // 创建wakeupfd 用来notify唤醒subReactor处理新来的channel
{
	int efd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if (efd < 0)
		LOG_FATAL("eventfd error:%d\n", errno);
	return efd;
}


EventLoop::EventLoop() : 
		_looping{ false }, _quit{ false }, _callingPendingFunctors{ false }, _threadId{ CurrentThread::tid() },
		_poller{ Poller::newDefaultPoller(this) }, _wakeupFd{ ::createEventfd() }, _wakeupChannel{ new Channel(this, _wakeupFd) }
{
	LOG_DEBUG("EventLoop created %p in thread %d\n", this, _threadId);
	if (::loopInThisThread)
		LOG_FATAL("Another EventLoop exists in this thread %d\n", ::loopInThisThread, _threadId);
	else
		::loopInThisThread = this;
	
	// 设置wakeupfd的事件类型以及发生事件后的回调操作
	_wakeupChannel->setReadCallback(std::bind(EventLoop::handleRead, this));

	// 每一个EventLoop都将监听_wakeupChannel的EPOLLIN事件
	_wakeupChannel->enableReading(); 
}


EventLoop::~EventLoop()
{
	_wakeupChannel->disableAll();	// 给Channel移除所有感兴趣的事件
	_wakeupChannel->remove();   	// 把Channel从EventLoop上删除掉
	::close(_wakeupFd);
	::loopInThisThread = nullptr;
}


void EventLoop::loop()
{
	_looping = true;
	_quit = false;	

	LOG_INFO("EventLoop %p start looping\n", this);
	while (!_quit)
	{
		_activeChannels.clear();
		_pollReturnTime = _poller->poll(::PollTimeout, _activeChannels);
		for (auto&& channel : _activeChannels)
			channel->handleEvent(_pollReturnTime); // Poller监听哪些channel发生了事件 然后上报给EventLoop 通知channel处理相应的事件
		/*
        	执行当前EventLoop事件循环需要处理的回调操作 对于线程数 >=2 的情况 IO线程 mainloop(mainReactor) 主要工作：
        	accept接收连接 => 将accept返回的connfd打包为Channel => TcpServer::newConnection通过轮询将TcpConnection对象分配给subloop处理, 
        	mainloop调用queueInLoop将回调加入subloop（该回调需要subloop执行 但subloop还在poller_->poll处阻塞）, 
			queueInLoop通过wakeup将subloop唤醒
        */
        doPendingFunctors();
	}
	LOG_INFO("EventLoop %p stop looping.\n", this);

	_looping = false;
}


void EventLoop::quit()
{
	_quit = true;
	if (!isInLoopThread())
		wakeup();
}


// 在当前loop中执行回调
void EventLoop::runInLoop(Functor cb)
{
	if (isInLoopThread())
		cb();
	else
		queueInLoop(cb);
}

// 把cb放入队列中 唤醒loop所在的线程执行cb
void EventLoop::queueInLoop(Functor cb)
{
	{
		std::lock_guard<std::mutex> guard(this->_pendingMtx);
		_pendingFunctors.push_back(std::move(cb));
	}

	/*
     * callingPendingFunctors的意思是 当前loop正在执行回调中 但是loop的_pendingFunctors中又加入了新的回调 需要通过wakeup写事件
     * 唤醒相应的需要执行上面回调操作的loop的线程 让loop()下一次_poller->poll()不再阻塞（阻塞的话会延迟前一次新加入的回调的执行），然后
     * 继续执行_pendingFunctors中的回调函数
    */
	if (!isInLoopThread() || _callingPendingFunctors)
		wakeup();		// 唤醒loop所在的线程
}

/// @brief 处理eventfd的读事件
void EventLoop::handleRead()
{
	uint64_t one = 1;
	ssize_t n = ::read(_wakeupFd, &one, sizeof(one));
	if (n != sizeof(one))
		LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8\n", n);
}

/// @brief 唤醒loop所在线程 向wakeupFd_写一个数据 wakeupChannel就发生读事件 当前loop线程就会被唤醒
void EventLoop::wakeup()
{
	uint64_t one = 1;
	ssize_t n = ::write(_wakeupFd, &one, sizeof(one));
	if (n != sizeof(one))
		LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8\n", n);
}


// EventLoop的方法 => Poller的方法
void EventLoop::removeChannel(Channel& channel)
{
	_poller->removeChannel(channel);
}

void EventLoop::updateChannel(Channel& channel)
{
	_poller->updateChannel(channel);
}


bool EventLoop::hasChannel(Channel& channel)
{
	return _poller->hasChannel(channel);
}

/// @brief 处理subloop上的待处理的回调函数
void EventLoop::doPendingFunctors()
{
	std::vector<Functor> functors;
	_callingPendingFunctors = true;

	{
		std::lock_guard<std::mutex> guard(_pendingMtx);
		// 用交换的方式减少了临界区范围 提升效率 同时避免了死锁 如果执行functor()在临界区内 且functor()中调用queueInLoop()就会产生死锁
		functors.swap(this->_pendingFunctors); 		// 交换两个vector的底层数据指针data
	}

	for (auto&& func : functors)
		func();  // 执行当前loop需要执行的回调操作
	
	_callingPendingFunctors = false;
}








