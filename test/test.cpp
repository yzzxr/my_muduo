#include <string>
#include <mymuduo/TcpServer.h>
#include <mymuduo/Logger.h>
#include <mymuduo/InetAddress.h>
#include <mymuduo/EventLoopThreadPool.h>
#include <mymuduo/CurrentThread.h>

/// @brief 
class EchoServer
{
public:
	EchoServer(EventLoop* loop, const InetAddress& addr, const std::string& name) : _server(loop, addr, name), _loop(loop)
	{
		// 注册回调函数
		_server.setConnectionCallback(std::bind(&EchoServer::onConnection, this, std::placeholders::_1));

		_server.setMessageCallback(std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

		// 设置合适的subloop线程数量
		_server.setThreadNum(3);
	}
	void start()
	{
		_server.start();
	}

private:
	// 连接建立或断开的回调函数
	void onConnection(const TcpConnectionPtr& conn)
	{
		if (conn->connected())
		{
			LOG_INFO("Connection UP : %s", conn->peerAddress().toIpPort().data());
		}
		else
		{
			LOG_INFO("Connection DOWN : %s", conn->peerAddress().toIpPort().data());
		}
	}

	// 可读写事件回调
	void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
	{
		std::string msg = buf->retrieveAllAsString();
		conn->send(msg);
		// conn->shutdown();   // 关闭写端 底层响应EPOLLHUP => 执行_closeCallback
	}

	EventLoop* _loop;
	TcpServer _server;
};

int main() 
{
	EventLoop loop;
	InetAddress addr(8088);
	EchoServer server(&loop, addr, "EchoServer");
	server.start();
	loop.loop();
}

