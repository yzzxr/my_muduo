#include "CurrentThread.h"

namespace CurrentThread
{
    thread_local int t_cachedTid = 0;  // 线程局部变量，每个线程都有一份拷贝，写操作不会产生竞态条件

    void cacheTid()
    {
        if (t_cachedTid == 0)
        {
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}