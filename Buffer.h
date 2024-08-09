#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <cstddef>

/// @brief 网络库底层的缓冲区类型定义
class Buffer
{
public:
	static constexpr size_t kCheapPrepend = 8; 		// 一个指针的大小
	static constexpr size_t kInitialSize = 1024;

	explicit Buffer(size_t initalSize = kInitialSize) : _buffer(kCheapPrepend + initalSize)
		, _readerIndex(kCheapPrepend)
		, _writerIndex(kCheapPrepend)
	{
	}

	size_t readableBytes() const { return _writerIndex - _readerIndex; }
	size_t writableBytes() const { return _buffer.size() - _writerIndex; }
	size_t prependableBytes() const { return _readerIndex; }

	// 返回缓冲区中可读数据的起始地址
	const char* peek() const { return begin() + _readerIndex; }

	void retrieve(size_t len)
	{
		if (len < readableBytes())
		{
			_readerIndex += len; // 说明应用只读取了可读缓冲区数据的一部分，就是len长度 还剩下readerIndex+=_len到writerIndex的数据未读
		}
		else // len == readableBytes()
		{
			retrieveAll();
		}
	}
	void retrieveAll()
	{
		_readerIndex = kCheapPrepend;
		_writerIndex = kCheapPrepend;
	}

	// 把onMessage函数上报的Buffer数据 转成string类型的数据返回
	std::string retrieveAllAsString() { return retrieveAsString(readableBytes()); }
	std::string retrieveAsString(size_t len)
	{
		std::string result(peek(), len);
		retrieve(len); // 上面一句把缓冲区中可读的数据已经读取出来 这里肯定要对缓冲区进行复位操作
		return result;
	}

	// _buffer.size - _writerIndex
	void ensureWritableBytes(size_t len)
	{
		if (writableBytes() < len)
		{
			makeSpace(len); // 扩容
		}
	}

	// 把[data, data+len]内存上的数据添加到writable缓冲区当中
	void append(const char* data, size_t len)
	{
		ensureWritableBytes(len);
		std::copy(data, data + len, beginWrite());
		_writerIndex += len;
	}
	char* beginWrite() { return begin() + _writerIndex; }
	const char* beginWrite() const { return begin() + _writerIndex; }

	// 从fd上读取数据
	ssize_t readFd(int fd, int* saveErrno);
	// 通过fd发送数据
	ssize_t writeFd(int fd, int* saveErrno);

private:
	// vector底层数组首元素的地址 也就是数组的起始地址
	char* begin() { return _buffer.data(); }
	const char* begin() const { return _buffer.data(); }

	void makeSpace(size_t len)
	{
		// xxx标示reader中已读的部分
		/**
		 * | kCheapPrepend |xxx| reader | writer |            
		 * | kCheapPrepend | reader ｜          len          |
		**/
		if (writableBytes() + prependableBytes() < len + kCheapPrepend) // 也就是说 len > xxx + writer的部分
		{
			_buffer.resize(_writerIndex + len);
		}
		else // 这里说明 len <= xxx + writer 把reader搬到从xxx开始 使得xxx后面是一段连续空间
		{
			size_t readable = readableBytes(); // readable = reader的长度
			std::copy(begin() + _readerIndex,
				begin() + _writerIndex,  // 把这一部分数据拷贝到begin+kCheapPrepend起始处
				begin() + kCheapPrepend);
			_readerIndex = kCheapPrepend;
			_writerIndex = _readerIndex + readable;
		}
	}

	std::vector<char> _buffer;  // 用动态数组作为缓冲区,可自动扩容
	size_t _readerIndex;  		// 读索引
	size_t _writerIndex;  		// 写索引
};
