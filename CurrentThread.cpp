#include "CurrentThread.h"

namespace CurrentThread
{
	thread_local int t_cachedTid = 0;  // 线程局部变量，每个线程都有一份拷贝，写操作不会产生竞态条件

	void cacheTid()
	{
		if (t_cachedTid == 0)
		{
			// 如果还没有获取过线程id，则通过系统调用来获取，获取过之后，每次只需读取这个变量的之就行了
			t_cachedTid = ::gettid();
		}
	}

	int tid()
	{
		// __builtin_expect 是一种底层优化 此语句意思是如果还未获取tid 进入if 通过cacheTid()系统调用获取tid
		if (__builtin_expect(t_cachedTid == 0, 0))
			cacheTid();
		return t_cachedTid;
	}
}