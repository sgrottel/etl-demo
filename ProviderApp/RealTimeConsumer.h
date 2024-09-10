#pragma once

class RealTimeConsumer
{
public:
	RealTimeConsumer();
	RealTimeConsumer(const RealTimeConsumer&) = delete;
	RealTimeConsumer(RealTimeConsumer&&) = delete;
	~RealTimeConsumer();
	RealTimeConsumer& operator=(const RealTimeConsumer&) = delete;
	RealTimeConsumer& operator=(RealTimeConsumer&&) = delete;
private:
};
