#pragma once

#include <vector>
#include <sys/epoll.h>

#include "Poller.h"
#include "Timestamp.h"

class Channel;


class EpollPoller : public Poller
{
public:
	EpollPoller(EventLoop* loop);
	virtual ~EpollPoller() override;

	// 重写基类的方法
	virtual Timestamp poll(int timeoutMs, ChannelList& activeChannels) override;
	virtual void updateChannel(Channel& channel) override;
	virtual void removeChannel(Channel& channel) override;

private:
	static const int initEventListSize = 16;

	void fillActiveChannels(int numEvents, ChannelList& activeChannels) const;  // 填写活跃的连接

	void update(int operation, Channel& channel);  // 更新channel通道 其实就是调用epoll_ctl

	int _epollfd;    		// epoll_create创建返回的fd保存在_epollfd中
	std::vector<epoll_event> _events;		// 用于存放epoll_wait返回的所有发生的事件的描述符事件集
};	