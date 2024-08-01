#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

// 封装ipv4的地址类型
class InetAddress
{
public:
	explicit InetAddress(ushort port = 0, const std::string& ip = "127.0.0.1");
	explicit InetAddress(const sockaddr_in& addr) : _addr(addr) {}

	std::string toIp() const;
	std::string toIpPort() const;
	ushort toPort() const;

	inline const sockaddr_in* getSockAddr() const { return &_addr; }
	inline void setSockAddr(const sockaddr_in& addr) { _addr = addr; }

private:
	sockaddr_in _addr;
};