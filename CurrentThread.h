#pragma once

#include <unistd.h>
#include <sys/syscall.h>


namespace CurrentThread
{
	extern thread_local int t_cachedTid;

	void cacheTid();

	int tid();
}



