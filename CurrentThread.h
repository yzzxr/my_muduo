#pragma once

#include <unistd.h>
#include <sys/syscall.h>


namespace CurrentThread
{
	extern thread_local int t_cachedTid;

	void cacheTid();

	inline int tid()
	{
		// __builtin_expect 是一种底层优化 此语句意思是如果还未获取tid 进入if 通过cacheTid()系统调用获取tid
		if (__builtin_expect(t_cachedTid == 0, 0))
			cacheTid();
		return t_cachedTid;
	}
}



