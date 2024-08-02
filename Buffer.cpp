#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>
#include "Buffer.h"


/**
 * 从fd上读取数据 Poller工作在LT模式
 * Buffer缓冲区是有大小的！ 但是从fd上读取数据的时候 却不知道tcp数据的最终大小
 *
 * 从socket读到缓冲区的方法是使用readv先读至_buffer，
 * _buffer空间如果不够会读入到栈上65536个字节大小的空间，然后以append的
 * 方式追加入_buffer。既考虑了避免系统调用带来开销，又不影响数据的接收。
 **/
ssize_t Buffer::readFd(int fd, int* saveErrno)
{
	// 栈额外空间，用于从套接字往出读时，当_buffer暂时不够用时暂存数据，待_buffer重新分配足够空间后，在把数据交换给_buffer。
	char extrabuf[65536] = { 0 }; // 栈上内存空间 65536/1024 = 64KB

	/*
	struct iovec {
		ptr_t iov_base; // iov_base指向的缓冲区存放的是readv所接收的数据或是writev将要发送的数据
		size_t iov_len; // iov_len在各种情况下分别确定了接收的最大长度以及实际写入的长度
	};
	*/

	// 使用iovec分配两个连续的缓冲区
	struct iovec vec[2];
	const size_t writable = writableBytes(); // 这是Buffer底层缓冲区剩余的可写空间大小 不一定能完全存储从fd读出的数据

	// 第一块缓冲区，指向可写空间
	vec[0].iov_base = begin() + _writerIndex;
	vec[0].iov_len = writable;

	// 第二块缓冲区，指向栈空间
	vec[1].iov_base = extrabuf;
	vec[1].iov_len = sizeof(extrabuf);

	// when there is enough space in this buffer, don't read into extrabuf.
	// when extrabuf is used, we read 128k-1 bytes at most.
	// 这里之所以说最多128k-1字节，是因为若writable为64k-1，那么需要两个缓冲区 第一个64k-1 第二个64k 所以做多128k-1
	// 如果第一个缓冲区>=64k 那就只采用一个缓冲区 而不使用栈空间extrabuf[65536]的内容
	const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
	const ssize_t n = ::readv(fd, vec, iovcnt);

	if (n < 0) 	// 读失败
	{
		*saveErrno = errno;
	}
	else if (n <= writable) // Buffer的可写缓冲区已经够存储读出来的数据了
	{
		_writerIndex += n;
	}
	else // extrabuf里面也写入了n-writable长度的数据
	{
		_writerIndex = _buffer.size();
		append(extrabuf, n - writable); // 对_buffer扩容 并将extrabuf存储的另一部分数据追加至_buffer
	}
	return n;
}

// input_buffer.readFd表示将对端数据读到input_buffer中，移动_writerIndex指针
// output_buffer.writeFd标示将数据写入到output_buffer中，从readerIndex_开始，可以写readableBytes()个字节
ssize_t Buffer::writeFd(int fd, int* saveErrno)
{
	ssize_t n = ::write(fd, peek(), readableBytes());
	if (n < 0)
	{
		*saveErrno = errno;
	}
	return n;
}