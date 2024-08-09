#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

class Channel;
class Poller;


/// @brief 事件循环，一个线程最多只有一个EventLoop, 主要包含了两个大模块 Channel Poller(epoll的抽象)
class EventLoop : public noncopyable
{
public:
	using Functor = std::function<void()>;

	EventLoop();
	~EventLoop();

	// 开启事件循环
	void loop();
	// 退出事件循环
	void quit();

	Timestamp pollReturnTime() const { return _pollReturnTime; }

	/// @brief 立即在当前loop中执行回调函数cb
	/// @param cb 
	void runInLoop(Functor cb);
	/// @brief 把上层注册的回调函数cb放入队列中 唤醒loop所在的线程执行cb
	/// @param cb 
	void queueInLoop(Functor cb);

	/// @brief 通过eventfd唤醒loop所在的线程
	void wakeup();

	// EventLoop的方法 => Poller的方法
	void updateChannel(Channel& channel);
	void removeChannel(Channel& channel);
	bool hasChannel(Channel& channel);

	/// @brief 判断EventLoop对象是否在自己所处的线程中, threadId_为EventLoop创建时的线程id CurrentThread::tid()为当前线程id
	/// @return 
	bool isInLoopThread() const 
	{
		return _threadId == CurrentThread::tid();
	}

private:
	// 给eventfd返回的文件描述符wakeupFd_绑定的事件回调 当wakeup()时 即有事件发生时 调用handleRead()读wakeupFd_的8字节 同时唤醒阻塞的epoll_wait
	void handleRead(Timestamp);
	// 执行上层回调
	void doPendingFunctors();

	using ChannelList = std::vector<Channel*>;

	std::atomic<bool> _looping; // 原子操作 底层通过CAS(比较并交换)实现
	std::atomic<bool> _quit;    // 标识退出loop循环

	const pid_t _threadId;  // 标识当前eventloop所属的线程id

	Timestamp _pollReturnTime;  // poller返回发生事件的Channel的时间点
	std::unique_ptr<Poller> _poller; // 一个EventLoop只有一个Poller，所以用独占指针

	int _wakeupFd; // 当mainLoop获取一个新用户的Channel 需通过轮询算法选择一个subLoop 通过该成员唤醒subLoop处理Channel
	std::unique_ptr<Channel> _wakeupChannel;	// 一个EventLoop只有一个wakeupfd，所以用独占指针

	ChannelList _activeChannels; // 返回Poller检测到的当前有事件发生的所有Channel的列表

	std::atomic<bool> _callingPendingFunctors;    	// 标识当前loop是否有需要执行的回调操作
	std::vector<Functor> _pendingFunctors;    		// 存储loop需要执行的所有回调操作
	std::mutex _pendingMtx;  						// 保护上面vector容器的线程安全操作
};
