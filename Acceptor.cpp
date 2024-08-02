#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"

/// @brief 创建非阻塞的、cloexec的tcp套接字
static int createNonBlocking()
{
	int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
	if (sockfd < 0)
		LOG_FATAL("%s:%s:%d listen socket create err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
	return sockfd;
}


Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport)
	: _loop{loop}, _acceptSocket{createNonBlocking()}, _acceptChannel(loop, _acceptSocket.fd()), _listening{false}
{
	_acceptSocket.setReuseAddr(true);
	_acceptSocket.setReusePort(true);
	_acceptSocket.bindAddress(listenAddr);
	// TcpServer::start() => Acceptor.listen() 如果有新用户连接 要执行一个回调(accept => connfd => 打包成Channel => 唤醒subloop)
    // mainloop监听到有事件发生 => acceptChannel_(listenfd) => 执行该回调函数
    _acceptChannel.setReadCallback(std::bind(&Acceptor::handleRead, this));
}


Acceptor::~Acceptor()
{
    _acceptChannel.disableAll();    // 把Poller中感兴趣的事件删除掉
    _acceptChannel.remove();        // 调用EventLoop->removeChannel => Poller->removeChannel 把Poller的ChannelMap对应的部分删除
}


void Acceptor::listen()
{
	_listening = true;
	_acceptSocket.listen();			// listen
	_acceptChannel.enableReading(); // _acceptChannel 注册至Poller !重要
}

// listenfd有事件发生就是有新用户连接了
void Acceptor::handleRead()
{
	InetAddress peerAddr;
	int connfd = _acceptSocket.accept(peerAddr);
	if (connfd >= 0)
	{
		// 轮询找到subLoop 唤醒并分发当前的新客户端的Channel
		if (_newConnectionCallback)
			_newConnectionCallback(connfd, peerAddr);
		else
			::close(connfd);
	}
	else
	{
		LOG_ERROR("%s:%s:%d accept err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
		if (errno == EMFILE)	
			LOG_ERROR("%s:%s:%d sockfd reached limit\n", __FILE__, __FUNCTION__, __LINE__);
	}
}



