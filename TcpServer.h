#pragma once

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>

#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include "TcpConnection.h"
#include "Buffer.h"

// 对外的服务器编程使用的类
class TcpServer : public noncopyable
{
public:
	using ThreadInitCallback = std::function<void(EventLoop*)>;

	enum class Option : int
	{
		NoReusePort,
		ReusePort
	};

	TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& name, Option option = Option::NoReusePort);
	~TcpServer();

	void setThreadInitCallback(ThreadInitCallback cb) { _threadInitCallback = std::move(cb); }
	void setConnectionCallback(ConnectionCallback cb) { _connectionCallback = std::move(cb); }
	void setMessageCallback(MessageCallback cb) { _messageCallback = std::move(cb); }
	void setWriteCompleteCallback(WriteCompleteCallback cb) { _writeCompleteCallback = std::move(cb); }


	// 设置底层subloop的个数
	void setThreadNum(int numThreads);

	// 开启服务器监听
	void start();

private:
	void newConnection(int sockfd, const InetAddress& peerAddr);
	void removeConnection(const TcpConnectionPtr& conn);
	void removeConnectionInLoop(const TcpConnectionPtr& conn);

	EventLoop* _loop;  // baseloop

	std::string _ipPort;
	std::string _name;

	std::unique_ptr<Acceptor> _acceptor; // 运行在mainloop 任务就是监听新连接事件
	std::shared_ptr<EventLoopThreadPool> _threadPool; // one loop per thread

	ConnectionCallback _connectionCallback;       	//有新连接时的回调
	MessageCallback _messageCallback;             	// 有读写事件发生时的回调
	WriteCompleteCallback _writeCompleteCallback; 	// 消息发送完成后的回调
	ThreadInitCallback _threadInitCallback; 		// loop线程初始化的回调

	std::atomic<int> _started;

	int _nextConnId;
	std::unordered_map<std::string, TcpConnectionPtr> _connections;  // 保存所有的连接
};




