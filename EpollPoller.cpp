#include <errno.h>
#include <unistd.h>
#include <cstring>

#include "EpollPoller.h"
#include "Logger.h"
#include "Channel.h"


constexpr int New = -1;    		// 某个channel还没添加至Poller, channel的成员_index初始化为New
constexpr int Added = 1;   		// 某个channel已经添加至Poller
constexpr int Deleted = 2; 		// 某个channel已经从Poller删除


EpollPoller::EpollPoller(EventLoop* loop) : Poller(loop), _epollfd{::epoll_create1(EPOLL_CLOEXEC)}, _events{initEventListSize}
{
	if (_epollfd < 0)
		LOG_FATAL("epoll_create error:%d \n", errno);
}


EpollPoller::~EpollPoller()
{
	::close(_epollfd);
}


Timestamp EpollPoller::poll(int timeoutMs, ChannelList& channelList)
{
	// 由于频繁调用poll 实际上应该用LOG_DEBUG输出日志更为合理 当遇到并发场景 关闭DEBUG日志提升效率
    LOG_DEBUG("func=%s => fd total count:%lu\n", __FUNCTION__, channels_.size());

	int numEvents = ::epoll_wait(_epollfd, _events.data(), static_cast<int>(_events.size()), timeoutMs);
	Timestamp now{Timestamp::now()};

	if (numEvents > 0)
	{
		LOG_DEBUG("%d events happend\n", numEvents);
		this->fillActiveChannels(numEvents, channelList);
		if (numEvents == _events.size())
			_events.resize(_events.size() * 2);
	}
	else if (numEvents == 0)
		LOG_DEBUG("%s timeout!\n", __FUNCTION__);
	else
	{
		if (errno == EINTR)
		{
			LOG_ERROR("EpollPoller::poll error!");
		}
	}
	return now;
}


// channel update remove => EventLoop updateChannel removeChannel => Poller updateChannel removeChannel
void EpollPoller::updateChannel(Channel& channel)
{
	const int index = channel.index();
	LOG_INFO("func=%s => fd=%d events=%d index=%d\n", __FUNCTION__, channel.fd(), channel.events(), index);

	if (index == New || index == Deleted)
	{
		if (index == New)
		{
			int fd = channel.fd();
			_channels[fd] = &channel;
		}
		channel.set_index(Added);
		update(EPOLL_CTL_ADD, channel);
	}
	else
	{
		int fd = channel.fd();
		if (channel.isNoneEvent())
		{
			update(EPOLL_CTL_DEL, channel);
			channel.set_index(Deleted);
		}
		else
			update(EPOLL_CTL_MOD, channel);
	}
}


/// @brief 从poller中删除channel
void EpollPoller::removeChannel(Channel& channel)
{
	int fd = channel.fd();
	_channels.erase(fd);

	LOG_INFO("func=%s => fd=%d\n", __FUNCTION__, fd);

	int index = channel.index();
	if (Added)
		update(EPOLL_CTL_DEL, channel);
	channel.set_index(New);
}


/// @brief 填写活跃的连接
/// @param numEvents 
/// @param activeChannels 
void EpollPoller::fillActiveChannels(int numEvents, ChannelList& activeChannels) const
{
	for (int i = 0; i < numEvents; i++)
	{
		Channel* channel = static_cast<Channel*>(_events[i].data.ptr);
		channel->set_revents(_events[i].events);
		activeChannels.push_back(channel);   // EventLoop就拿到了它的Poller给它返回的所有发生事件的channel列表了
	}
}


/// @brief 更新channel通道 其实就是调用epoll_ctl add/mod/del
/// @param operation 
/// @param channel 
void EpollPoller::update(int operation, Channel& channel)
{
	epoll_event event;
	::memset(&event, 0, sizeof(event));

	int fd = channel.fd();

	event.data.fd = fd;
	event.data.ptr = &channel;
	event.events = channel.events();

	if (::epoll_ctl(_epollfd, operation, fd, &event) < 0)
	{
		if (operation == EPOLL_CTL_DEL)
		{
			LOG_ERROR("epoll_ctl del error:%d\n", errno);
		}
		else
		{
			LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
		}
	}
}


