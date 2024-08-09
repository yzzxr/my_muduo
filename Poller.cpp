#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

#include "EventLoop.h"
#include "Logger.h"
#include "Channel.h"
#include "Poller.h"


Poller::Poller(EventLoop* loop) : _ownerLoop{loop}
{

}



bool Poller::hasChannel(Channel& channel)
{
	return _channels.contains(channel.fd());
}







