#pragma once

#include <memory>
#include <functional>
#include <atomic>
#include <sys/epoll.h>

#include "noncopyable.h"
#include "Timestamp.h"	

class EventLoop;

/// @brief Channel理解为频道,封装了sockfd和其感兴趣的event 如EPOLLIN、EPOLLOUT事件 
/// 还绑定了poller返回的具体事件, 相当于Reactor模型上对应多路事件分发器
class Channel : noncopyable
{
public:
	using EventCallback = std::function<void()>;
	using ReadEventCallback = std::function<void(Timestamp)>;

	~Channel() = default;
	Channel(EventLoop* loop, int fd);

	// fd得到Poller通知以后 处理事件 handleEvent在EventLoop::loop()中调用
	void handleEvent(Timestamp receiveTime);

	/// @brief 设置四个回调函数
	void setReadCallback(ReadEventCallback cb) { _readCallback = std::move(cb); }
	void setWriteCallback(EventCallback cb) { _writeCallback = std::move(cb); }
	void setCloseCallback(EventCallback cb) { _closeCallback = std::move(cb); }
	void setErrorCallback(EventCallback cb) { _errorCallback = std::move(cb); }

	// 防止当channel被手动remove掉 channel还在执行回调操作
	void tie(const std::shared_ptr<void>&);

	inline int fd() const { return _fd; }
	inline int events() const { return _events; }
	inline void set_revents(int data) { _revents = data; }

	// 设置_fd相应的感兴趣事件状态, 相当于epoll_ctl, add, delete
	void enableReading() { _events |= ReadEvent; update(); }
	void disableReading() { _events &= ~ReadEvent; update(); }
	void enableWriting() { _events |= WriteEvent; update(); }
	void disableWriting() { _events &= ~WriteEvent; update(); }
	void disableAll() { _events = NoneEvent; update(); }

	// 返回fd当前的事件状态
	bool isNoneEvent() const { return _events == NoneEvent; }
	bool isWriting() const { return _events & WriteEvent; }
	bool isReading() const { return _events & ReadEvent; }

	inline int index() const { return _index; }
	inline void set_index(int idx) { _index = idx; }

	EventLoop* onwerLoop() const { return _loop; }
	void remove();
private:
	void update();
	void handleEventWithGuard(Timestamp receiveTime);

	// 三个特殊事件，用编译期常量标注
	inline static constexpr int NoneEvent = 0;
	inline static constexpr int ReadEvent = EPOLLIN | EPOLLPRI;
	inline static constexpr int WriteEvent = EPOLLOUT;

	EventLoop* _loop;	// 该Channel所属的事件循环
	const int _fd;  	// 封装的套接字描述符
	int _events;     	// fd感兴趣事件
	int _revents;		// 实际发生的事件
	int _index;			// 标记Channel的创建状态

	// Tcpconnection封装了一个Channel, 处理Tcpconnection生命周期先于Channel结束的情况
	std::weak_ptr<void> _tie;
	std::atomic<bool> _tied;

	ReadEventCallback _readCallback;	// 描述符上有可读事件
	EventCallback _writeCallback;		// 描述符上有可写事件
	EventCallback _closeCallback;		// 对端关闭连接事件
	EventCallback _errorCallback;		// 描述符上产生出错误
};

