#include "CurrentThread.h"

namespace CurrentThread
{
    thread_local int t_cachedTid = 0;  // 线程局部变量，每个线程都有一份拷贝，写操作不会产生竞态条件

    void cacheTid()
    {
        if (t_cachedTid == 0)
        {
			// 如果还没有获取过线程id，则通过系统调用来获取，获取过之后，每次只需读取这个变量的之就行了
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}