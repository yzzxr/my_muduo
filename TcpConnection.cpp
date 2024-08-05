#include <functional>
#include <string>
#include <cerrno>
#include <sys/types.h>
#include <sys/socket.h>
#include <cstring>
#include <netinet/tcp.h>

#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"


/// @brief 检查并返回一个eventloop
/// @param loop 
/// @return 
static EventLoop* CheckLoopNotNull(EventLoop* loop)
{
	if (loop == nullptr)
		LOG_FATAL("%s:%s:%d mainLoop is null!\n", __FILE__, __FUNCTION__, __LINE__);
	return loop;
}


TcpConnection::TcpConnection(EventLoop* loop, const std::string& name, int sockfd, const InetAddress& local, const InetAddress& remote)
	: _loop{ CheckLoopNotNull(loop) }, _name{ name }, _state{ StateE::Connecting }, _reading{ true }, _socket{ new Socket(sockfd) },
	_channel{ new Channel(loop, sockfd) }, _localAddr{ local }, _peerAddr{ remote }, _highWaterMark{ 64 * 1024 * 1024 }
{
	// 给channel设置相应的回调函数, poller给channel通知感兴趣的事件发生了, channel会调用相应的回调函数
	_channel->setReadCallback(std::bind(TcpConnection::handleRead, this, std::placeholders::_1));
	_channel->setWriteCallback(std::bind(TcpConnection::handleWrite, this));
	_channel->setCloseCallback(std::bind(TcpConnection::handleClose, this));
	_channel->setErrorCallback(std::bind(TcpConnection::handleError, this));

	LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", _name.data(), sockfd);
	_socket->setKeepAlive(true);  // 开启tcp探测保活机制
}



TcpConnection::~TcpConnection()
{
	LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d\n", _name.data(), _channel->fd(), _state);
}



void TcpConnection::send(const std::string& buf)
{
	if (_state == StateE::Connected)
	{
		if (_loop->isInLoopThread())
			sendInLoop(buf.data(), buf.size());
		else
			_loop->runInLoop(std::bind(TcpConnection::sendInLoop, this, buf.data(), buf.size()));
	}
}


// 发送数据 应用写的快 而内核发送数据慢 需要把待发送数据写入缓冲区，而且设置了水位回调
void TcpConnection::sendInLoop(const void* data, size_t len)
{
	
}

