#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <cstring>
#include <netinet/tcp.h>

#include "Socket.h"
#include "Logger.h"
#include "InetAddress.h"

Socket::~Socket()
{
	::close(_sockfd);
}

void Socket::bindAddress(const InetAddress& localaddr)
{
	if (::bind(_sockfd, (sockaddr*)localaddr.getSockAddr(), sizeof(sockaddr_in)) < 0)
	{
		LOG_FATAL("bind sockfd:%d fail\n", _sockfd);
	}
}

void Socket::listen()
{
	if (::listen(_sockfd, 1024) < 0)
	{
		LOG_FATAL("listen sockfd:%d fail\n", _sockfd);
	}
}

int Socket::accept(InetAddress& peeraddr)
{
	/**
	 * 1. accept函数的参数不合法
	 * 2. 对返回的connfd没有设置非阻塞
	 * Reactor模型 one loop per thread
	 * poller + non-blocking IO
	 **/
	sockaddr_in addr;
	socklen_t len = sizeof(addr);
	::memset(&addr, 0, sizeof(addr));
	// 对已连接套接字直接设定非阻塞和close_exec
	int connfd = ::accept4(_sockfd, (sockaddr*)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
	if (connfd >= 0)
		peeraddr.setSockAddr(addr);
	return connfd;
}

void Socket::shutdownWrite()
{
	if (::shutdown(_sockfd, SHUT_WR) < 0)
		LOG_ERROR("shutdownWrite error");
}

void Socket::setTcpNoDelay(bool on)  // 禁用nagle算法
{
	int optval = on ? 1 : 0;
	::setsockopt(_sockfd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)); // TCP_NODELAY包含头文件 <netinet/tcp.h>
}
void Socket::setReuseAddr(bool on)
{
	int optval = on ? 1 : 0;
	::setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)); // TCP_NODELAY包含头文件 <netinet/tcp.h>
}
void Socket::setReusePort(bool on)
{
	int optval = on ? 1 : 0;
	::setsockopt(_sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)); // TCP_NODELAY包含头文件 <netinet/tcp.h>
}
void Socket::setKeepAlive(bool on)  // tcp保活探测机制开启
{
	int optval = on ? 1 : 0;
	::setsockopt(_sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)); // TCP_NODELAY包含头文件 <netinet/tcp.h>
}



