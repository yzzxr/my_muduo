#include <functional>
#include <string.h>

#include "TcpServer.h"
#include "Logger.h"
#include "TcpConnection.h"

using namespace std::placeholders;


static EventLoop* checkLoopNotNull(EventLoop* loop)
{
	if (loop == nullptr)
		LOG_FATAL("%s:%s:%d mainLoop is null!\n", __FILE__, __FUNCTION__, __LINE__);
	return loop;
}

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& name, Option option) :
	_loop{ checkLoopNotNull(loop) }, _ipPort{ listenAddr.toIpPort() }, _name{ name }, _acceptor{ new Acceptor(loop, listenAddr, option == Option::ReusePort) },
	_threadPool{ new EventLoopThreadPool(loop, name) }, _nextConnId{ 1 }, _started{ 0 }
{
	// 当有新用户连接时，Acceptor类中绑定的_acceptChannel会有读事件发生，执行handleRead()调用TcpServer::newConnection回调
	_acceptor->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, _1, _2));
}


TcpServer::~TcpServer()
{
	for (auto&& [name, ptr] : _connections)
	{
		TcpConnectionPtr conn(ptr);
		ptr.reset(); // 把原始的智能指针复位 让栈空间的TcpConnectionPtr conn指向该对象 当conn出了其作用域 即可释放智能指针指向的对象
		conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));  // 销毁连接
	}
}


void TcpServer::setThreadNum(int num)
{
	_threadPool->setThreadNum(num);
}


void TcpServer::start()
{
	// 防止一个TcpServer对象被start多次
	if (_started++ == 0)
	{
		_threadPool->start(_threadInitCallback);  // 启动线程池
		_loop->runInLoop(std::bind(&Acceptor::listen, _acceptor.get()));
	}
}


// 有一个新用户连接，acceptor会执行这个回调操作，负责将mainLoop接收到的请求连接(acceptChannel_会有读事件发生)通过回调轮询分发给subLoop去处理
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
	// 轮询算法 选择一个subLoop 来管理connfd对应的channel
	EventLoop* ioLoop = _threadPool->getNextLoop();
	char buf[64] = { 0 };
	snprintf(buf, sizeof(buf), "-%s#%d", _ipPort.data(), _nextConnId);
	_nextConnId++;  // 这里没有设置为原子类是因为其只在mainloop中执行 不涉及线程安全问题
	std::string connName = _name.append(buf);

	LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s\n", _name.data(), connName.data(), peerAddr.toIpPort().data());

	// 通过sockfd获取其绑定的本机的ip地址和端口信息
	sockaddr_in local;
	::memset(&local, 0, sizeof(local));
	socklen_t addrlen = sizeof(local);
	if (::getsockname(sockfd, (sockaddr*)&local, &addrlen) < 0)
		LOG_ERROR("sockets::getLocalAddr");
	
	InetAddress localAddr(local);
	TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
	_connections[connName] = conn;

	// 下面的回调都是用户设置给TcpServer => TcpConnection的，至于Channel绑定的则是TcpConnection设置的四个，handleRead,handleWrite... 这下面的回调用于handlexxx函数中
	conn->setConnectionCallback(_connectionCallback);
	conn->setMessageCallback(_messageCallback);
	conn->setWriteCompleteCallback(_writeCompleteCallback);

	// 设置了如何关闭连接的回调
	conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, _1));

	ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}


void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
	_loop->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}


void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
	LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n", _name.data(), conn->name().data());
	_connections.erase(conn->name());
	EventLoop* ioLoop = conn->getLoop();
	ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}



