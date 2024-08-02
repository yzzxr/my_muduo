#include <sys/epoll.h>

#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"


Channel::Channel(EventLoop* loop, int fd) : 
				_fd{fd}, _loop{loop}, 
				_events{NoneEvent}, _revents{NoneEvent}, 
				_index{-1}, _tied{false} 
{

}

/*
 * channel的tie方法什么时候调用?  TcpConnection => channel
 * TcpConnection中定义了Chnanel对应的回调函数，传入的回调函数均为TcpConnection
 * 对象的成员方法，因此可以说明一点就是：Channel的结束一定早于TcpConnection对象！
 * 此处用tie去解决TcoConnection和Channel的生命周期时长问题，从而保证了Channel对
 * 象能够在TcpConnection销毁前销毁。
*/
void Channel::tie(const std::shared_ptr<void>& other)
{
	_tie = other;
	_tied = true;
}

// 当改变channel所封装的fd的events事件后，update负责在poller里面更改fd相应的事件epoll_ctl
void Channel::update()
{
	// 通过channel所属的eventloop，调用poller的相应方法，更新fd的感兴趣events
	_loop->updateChannel(this);
}


// 在channel所属的EventLoop中把当前的channel删除掉
void Channel::remove()
{
	_loop->removeChannel(this);
}


void Channel::handleEventWithGuard(Timestamp receiveTime)
{
	LOG_INFO("channel handleEvent revents:%d\n", _revents);

	// 关闭
	if ((_revents & EPOLLHUP) && !(_events & EPOLLIN))
	{
		// 当TcpConnection对应Channel 通过shutdown 关闭写端 epoll触发EPOLLHUP
		if (_closeCallback)
			_closeCallback();
	}

	// 错误
	if (_revents & EPOLLERR)
	{
		if (_errorCallback)
			_errorCallback();
	}

	// 读事件
	if (_revents & EPOLLIN)
	{
		if (_readCallback)
			_readCallback(receiveTime);
	}

	// 写
	if (_revents & EPOLLOUT)
	{
		if (_writeCallback)
			_writeCallback();
	}
}



void Channel::handleEvent(Timestamp receiveTime)
{
	if (_tied)
	{
		std::shared_ptr<void> gurad = _tie.lock();
		if (gurad)
			handleEventWithGuard(receiveTime);
	}
	else
	{
		handleEventWithGuard(receiveTime);
	}
}






