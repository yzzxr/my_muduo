#pragma once

#include "noncopyable.h"

class InetAddress;

// 封装socketfd
class Socket : noncopyable
{
public:
	explicit inline Socket(int sockfd) : _sockfd(sockfd) {}
	~Socket();

	int fd() const { return _sockfd; }
	void bindAddress(const InetAddress& localaddr);
	void listen();
	int accept(InetAddress* peeraddr);

	void shutdownWrite();

	void setTcpNoDelay(bool on);
	void setReuseAddr(bool on);
	void setReusePort(bool on);
	void setKeepAlive(bool on);

private:
	const int _sockfd;
};