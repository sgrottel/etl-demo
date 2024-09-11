#pragma once

#include <guiddef.h>
#include <memory>
#include <functional>

typedef struct _EVENT_RECORD* PEVENT_RECORD;

class RealTimeConsumer
{
public:
	RealTimeConsumer(const wchar_t* sessionName);
	RealTimeConsumer(const RealTimeConsumer&) = delete;
	RealTimeConsumer(RealTimeConsumer&&) = delete;
	~RealTimeConsumer();
	RealTimeConsumer& operator=(const RealTimeConsumer&) = delete;
	RealTimeConsumer& operator=(RealTimeConsumer&&) = delete;

	bool EnableTraceProviderCapture(GUID const& guid);

	void SetOnTraceEventCallback(std::function<void(PEVENT_RECORD)> func);

private:
	struct Data;
	std::unique_ptr<Data> m_data;
};
