# C++11 Muduo网络库

<img src="./img/show.png" style="float:top" width=870px height=570px/>


## 开发环境
* Linux WSL 5.15.153.1-microsoft-standard-WSL2
* gcc version `11.4.0`
* cmake version `3.29.2`

项目编译执行`./build.sh`即可，测试用例进入`example/`文件夹，`make`即可生成服务器测试用例

## 功能介绍

头文件生成至目录`/usr/include/mymuduo/`，`.so`动态库文件生成至目录`/usr/lib/`。

1. `EventLoop.*`、`Channel.*`、`Poller.*`、`EPollPoller.*`等主要用于事件轮询检测，并实现了事件分发处理的方法。`EventLoop`负责轮询执行`Poller`，要进行读、写、错误、关闭等事件时需执行哪些回调函数，均绑定至`Channel`中，事件发生后进行相应的回调处理即可
2. `Thread.*`、`EventLoopThread.*`、`EventLoopThreadPool.*`等将线程和`EventLoop`事件轮询绑定在一起，实现真正意义上的`one loop per thread`
3. `TcpServer.*`、`TcpConnection.*`、`Acceptor.*`、`Socket.*`等是`mainloop`对网络连接的响应并轮询分发至各个`subloop`的实现，其中注册大量回调函数
4. `Buffer.*`为`muduo`网络库自行设计的自动扩容的缓冲区，保证数据有序到达


## 项目亮点

1. `EventLoop`中使用了`eventfd`来调用`wakeup()`，让`mainloop`唤醒`subloop`的`epoll_wait()`
2. 在`EventLoop`中注册回调`cb`至`_pendingFunctors`，并在`doPendingFunctors`中通过`swap()`的方式，快速换出注册的回调，只在`swap()`时加锁，减少临界区代码的长度，提升多线程效率。（若不通过`swap()`的方式去处理，则会出现临界区过大,需要等待所有pendingfunctor执行完毕才释放锁,降低了服务器响应效率 2. 若执行的回调中执行`queueInLoop`需要抢占锁时，会发生死锁）
3. `Logger`可以设置日志等级，调试代码时可以开启`DEBUG`打印日志；若启动服务器，由于日志会影响服务器性能，可适当关闭`DEBUG`相关日志输出
4. 在`Thread`中通过`C++ lambda`表达式以及信号量机制保证线程创建时的有序性，只有当线程获取到了其自己的`tid`后，才算启动线程完毕
5. `TcpConnection`继承自`enable_shared_from_this`模板，`TcpConnection`对象可以调用`shared_from_this()`方法使用智能指针安全的产生shared_ptr，正确的控制引用计数，同时`muduo`通过`tie()`方式解决了`TcpConnection`对象生命周期先于`Channel`结束的情况
6. `muduo`采用`Reactor`模型和多线程结合的方式，实现了主要以`同步非阻塞IO`为主的网络库


