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

using namespace std::placeholders;

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
	_channel->setReadCallback(std::bind(&TcpConnection::handleRead, this, _1));
	_channel->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
	_channel->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
	_channel->setErrorCallback(std::bind(&TcpConnection::handleError, this));

	LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", _name.data(), sockfd);
	_socket->setKeepAlive(true);  // 开启tcp探测保活机制
}



TcpConnection::~TcpConnection()
{
	LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d\n", _name.data(), _channel->fd(), (int)_state);
}



void TcpConnection::send(const std::string& buf)
{
	if (_state == StateE::Connected)
	{
		if (_loop->isInLoopThread())
			sendInLoop(buf.data(), buf.size());
		else
			_loop->runInLoop(std::bind(&TcpConnection::sendInLoop, this, buf.data(), buf.size()));
	}
}


// 发送数据 应用写的快 而内核发送数据慢 需要把待发送数据写入缓冲区，而且设置了水位回调
void TcpConnection::sendInLoop(const void* data, size_t len)
{
	ssize_t nwrite = 0;
	size_t remaining = len;
	bool faultError = false;

	// 之前调用过该connection的shutdown 不能再进行发送了
	if (_state == StateE::Disconnected)
		LOG_ERROR("disconnected, give up writing!");

	// 表示_channel第一次开始写数据或者缓冲区没有待发送数据
	if (!_channel->isWriting() && _outputBuffer.readableBytes() == 0)
	{
		nwrite = ::write(_channel->fd(), data, len);
		if (nwrite >= 0)
		{
			remaining = len - nwrite;
			if (remaining == 0 && _writeCompleteCallback)
				// 既然在这里数据全部发送完成，就不用再给channel设置epollout事件了
				_loop->queueInLoop(std::bind(_writeCompleteCallback, shared_from_this()));
		}
		else
		{
			nwrite = 0;
			if (errno != EWOULDBLOCK)
			{
				LOG_ERROR("TcpConnection::sendInLoop");
				if (errno == EPIPE || errno == ECONNRESET)
					faultError = true;
			}
		}
	}

	/**
	 * 说明当前这一次write并没有把数据全部发送出去 剩余的数据需要保存到缓冲区当中
	 * 然后给channel注册EPOLLOUT事件，Poller发现tcp的发送缓冲区有空间后会通知
	 * 相应的sock->channel，调用channel对应注册的writeCallback_回调方法，
	 * channel的writeCallback_实际上就是TcpConnection设置的handleWrite回调，
	 * 把发送缓冲区outputBuffer_的内容全部发送完成
	**/
	if (!faultError && remaining > 0)
	{
		// 目前发送缓冲区剩余的待发送的数据的长度
		size_t oldLen = _outputBuffer.readableBytes();
		if (oldLen + remaining >= _highWaterMark && oldLen < _highWaterMark && _highWaterMarkCallback)
			_loop->queueInLoop(std::bind(_highWaterMarkCallback, shared_from_this(), oldLen + remaining));
		_outputBuffer.append((char*)data + nwrite, remaining);
		if (!_channel->isWriting())
			_channel->enableWriting(); // 一定要注册channel的写事件 否则poller不会给channel通知epollout
	}
}


void TcpConnection::shutdown()
{
	if (_state == StateE::Connected)
	{
		setState(StateE::Disconnecting);
		_loop->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
	}
}


void TcpConnection::shutdownInLoop()
{
	// 说明当前outputBuffer_的数据全部向外发送完成
	if (!_channel->isWriting()) 
	{
		_socket->shutdownWrite();
	}
}

// 连接建立
void TcpConnection::connectEstablished()
{
	setState(StateE::Connected);
	_channel->tie(shared_from_this());
	_channel->enableReading(); // 向poller注册channel的EPOLLIN事件

	// 新连接建立 执行回调
	_connectionCallback(shared_from_this());
}


// 连接销毁
void TcpConnection::connectDestroyed()
{
	if (_state == StateE::Connected)
	{
		setState(StateE::Disconnected);
		_channel->disableAll(); // 把channel的所有感兴趣的事件从poller中删除掉
		_connectionCallback(shared_from_this());
	}
	_channel->remove(); // 把channel从poller中删除掉
}


// 读是相对服务器而言的 当对端客户端有数据到达 服务器端检测到EPOLLIN 就会触发该fd上的回调 handleRead取读走对端发来的数据
void TcpConnection::handleRead(Timestamp receiveTime)
{
	int saveError = 0;
	ssize_t n = _inputBuffer.readFd(_channel->fd(), &saveError);
	if (n > 0)
	{
		// 已建立连接的用户有可读事件发生了 调用用户传入的回调操作onMessage shared_from_this就是获取了TcpConnection的智能指针
		_messageCallback(shared_from_this(), &_inputBuffer, receiveTime);
	}
	else if (n == 0)  // 对端断开连接
		handleClose();
	else  		// error
	{
		errno = saveError;
		LOG_ERROR("TcpConnection::handleRead");
		handleError();
	}
}


void TcpConnection::handleWrite()
{
	if (_channel->isWriting())
	{
		int saveError = 0;
		ssize_t n = _outputBuffer.writeFd(_channel->fd(), &saveError);
		if (n > 0)
		{
			_outputBuffer.retrieve(n);
			if (_outputBuffer.readableBytes() == 0)
			{
				_channel->disableWriting();
				if (_writeCompleteCallback)  // TcpConnection对象在其所在的subloop中 向pendingFunctors_中加入回调
					_loop->queueInLoop(std::bind(_writeCompleteCallback, shared_from_this()));
				if (_state == StateE::Disconnecting)
					shutdownInLoop();  		// 在当前所属的loop中把TcpConnection删除掉
			}
		}
		else
		{
			LOG_ERROR("TcpConnection::handleWrite");
		}
	}
	else
	{
		LOG_ERROR("TcpConnection fd=%d is down, no more writing", _channel->fd());
	}
}


void TcpConnection::handleClose()
{
	LOG_INFO("TcpConnection::handleClose fd=%d state=%d\n", _channel->fd(), (int)_state);
	setState(StateE::Disconnected);
	_channel->disableAll();

	TcpConnectionPtr connPtr(shared_from_this());
	_connectionCallback(connPtr); 			// 执行连接关闭的回调
	_closeCallback(connPtr);      			// 执行关闭连接的回调 执行的是TcpServer::removeConnection回调方法   // must be the last line
}


void TcpConnection::handleError()
{
	int optval = 0;
	socklen_t optlen = sizeof(optval);
	int err = 0;

	if (::getsockopt(_channel->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
		err = errno;
	else
		err = optval;
		
	LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d\n", _name.c_str(), err);
}




