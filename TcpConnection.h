#pragma once

#include <memory>
#include <string>
#include <atomic>

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"

class Channel;
class EventLoop;
class Socket;


/*
 * TcpServer => Acceptor => 有一个新用户连接，通过accept函数拿到connfd
 * => TcpConnection设置回调 => 设置到Channel => Poller => Channel回调
*/
class TcpConnection : public noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
	TcpConnection(EventLoop* loop, const std::string& nameArg, int sockfd, const InetAddress& local, const InetAddress& peer);
	~TcpConnection();

	EventLoop* getLoop() const { return _loop; }
	const std::string& name() const { return _name; }
	const InetAddress& localAddress() const { return _localAddr; }
	const InetAddress& peerAddress() const { return _peerAddr; }

	bool connected() const { return _state == StateE::Connected; }

	// 发送数据
	void send(const std::string& buf);
	// 关闭连接
	void shutdown();


	void setConnectionCallback(ConnectionCallback cb)
	{
		_connectionCallback = std::move(cb);
	}
	void setMessageCallback(MessageCallback cb)
	{
		_messageCallback = std::move(cb);
	}
	void setWriteCompleteCallback(WriteCompleteCallback cb)
	{
		_writeCompleteCallback = std::move(cb);
	}
	void setCloseCallback(CloseCallback cb)
	{
		_closeCallback = std::move(cb);
	}
	void setHighWaterMarkCallback(HighWaterMarkCallback cb, size_t highWaterMark)
	{
		_highWaterMarkCallback = std::move(cb); 
		_highWaterMark = highWaterMark;
	}

	// 连接建立
	void connectEstablished();
	// 连接销毁
	void connectDestroyed();

private:
	enum class StateE : int
	{
		Disconnected, 		// 已经断开连接
		Connecting,   		// 正在连接
		Connected,    		// 已连接
		Disconnecting 		// 正在断开连接
	};

	void setState(StateE state) { _state = state; }

	void handleRead(Timestamp receiveTime);
	void handleWrite();
	void handleClose();
	void handleError();

	void sendInLoop(const void* data, size_t len);
	void shutdownInLoop();

	// 这里是baseloop还是subloop由TcpServer中创建的线程数决定, 若为多Reactor 该loop_指向subloop 若为单Reactor 该loop_指向baseloop
	EventLoop* _loop;
	const std::string _name;
	std::atomic<StateE> _state;
	bool _reading;

	// Socket Channel 这里和Acceptor类似  Acceptor => mainloop  TcpConnection => subloop
	std::unique_ptr<Socket> _socket;
	std::unique_ptr<Channel> _channel;

	const InetAddress _localAddr;
	const InetAddress _peerAddr;

	// 这些回调TcpServer也有 用户通过写入TcpServer注册 TcpServer再将注册的回调传递给TcpConnection TcpConnection再将回调注册到Channel中
	ConnectionCallback _connectionCallback;
	MessageCallback _messageCallback;
	WriteCompleteCallback _writeCompleteCallback;
	HighWaterMarkCallback _highWaterMarkCallback;
	CloseCallback _closeCallback;
	size_t _highWaterMark;

	// 数据缓冲区,用户态的缓冲区
	Buffer _inputBuffer;    // 接收缓冲区
	Buffer _outputBuffer;   // 发送缓冲区 用户send向_outputBuffer发送
};

