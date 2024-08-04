#pragma once

#include <vector>
#include <unordered_map>

#include "noncopyable.h"
#include "Timestamp.h"

class Channel;
class EventLoop;

/// @brief 定义了IO多路复用器的核心接口的抽象类
class Poller
{
public:
	using ChannelList = std::vector<Channel*>;

	// 给所有IO多路复用定义统一的接口
	virtual Timestamp poll(int timeoutMs, ChannelList& activeChannels) = 0;
	virtual void updateChannel(Channel& channel) = 0;
	virtual void removeChannel(Channel& channel) = 0;

	bool hasChannel(Channel& channel) const; // 判断参数channel是否在当前的Poller当中

	static Poller* newDefaultPoller(EventLoop* loop); // EventLoop可以通过该接口获取默认的IO复用的具体实现

protected:
	using ChannelMap = std::unordered_map<int, Channel*>;
	ChannelMap _channels;

	Poller(EventLoop* loop);
	virtual ~Poller() = default;
private:
	EventLoop* _ownerLoop; // Poller所属的事件循环EventLoop
};	

