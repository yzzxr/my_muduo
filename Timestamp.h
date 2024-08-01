#pragma once

#include <iostream>
#include <string>

/// @brief 时间戳，获取当前时间
class Timestamp
{
public:
	Timestamp();
	explicit Timestamp(int64_t microSecondsSinceEpoch);
	static Timestamp now();
	std::string toString() const;
private:
	int64_t _microSecondsSinceEpoch;
};