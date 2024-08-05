#pragma once

#include <functional>

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"
#include "InetAddress.h"

class EventLoop;

/// @brief mainreadctor才会有Acceptor，subreactor只有TcpConnection
class Acceptor : public noncopyable
{
public:
	using NewConnectionCallback = std::function<void(int sockfd, const InetAddress& addr)>;

	Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
	~Acceptor();
	
	inline void setNewConnectionCallback(NewConnectionCallback cb) { _newConnectionCallback = std::move(cb); }

	inline bool listening() const { return _listening; }
	void listen();

private:
	void handleRead();

	NewConnectionCallback _newConnectionCallback;
	EventLoop* _loop;
	Socket _acceptSocket;
	Channel _acceptChannel; // 存放listen套接字
	bool _listening;
};


