#include <cstring>
#include <string>
#include "InetAddress.h"

InetAddress::InetAddress(ushort port, const std::string& ip)
{
	::memset(&_addr, 0, sizeof(_addr));
	_addr.sin_family = AF_INET;  // ipv4
	_addr.sin_port = ::htons(port); // 本地字节序转为网络字节序
	_addr.sin_addr.s_addr = ::inet_addr(ip.data());
}

std::string InetAddress::toIp() const
{
	std::string ip(::inet_ntoa(_addr.sin_addr));
	return ip;
}

std::string InetAddress::toIpPort() const
{
	std::string ip_port(::inet_ntoa(_addr.sin_addr));
	ushort port = ::ntohs(_addr.sin_port);
	ip_port.push_back(':');
	ip_port.append(std::to_string(port));
	return ip_port;
}

ushort InetAddress::toPort() const
{
	return ::ntohs(_addr.sin_port);
}


// #include <iostream>
// int main()
// {
// 	InetAddress addr(12345);
// 	std::cout << addr.toIp() << std::endl;
// 	std::cout << addr.toPort() << std::endl;
// 	std::cout << addr.toIpPort() << std::endl;
// }
