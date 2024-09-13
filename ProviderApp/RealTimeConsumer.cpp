#include "RealTimeConsumer.h"

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Objbase.h>
#include <evntrace.h>
#include <evntcons.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <thread>

struct RealTimeConsumer::Data
{
	std::vector<uint8_t> sessionPropertiesBuffer;
	EVENT_TRACE_PROPERTIES* sessionProperties = nullptr;
	TRACEHANDLE handle = 0;
	GUID sessionGuid{};
	std::wstring sessionName;

	EVENT_TRACE_LOGFILEW traceLog{};
	TRACEHANDLE hprocess = INVALID_PROCESSTRACE_HANDLE;
	std::thread hprocessWorker;

	std::function<void(PEVENT_RECORD)> onTraceEvent;

	static void WINAPI OnTraceEvent(PEVENT_RECORD EventRecord)
	{
		auto cb = static_cast<RealTimeConsumer::Data*>(EventRecord->UserContext)->onTraceEvent;
		if (cb) { cb(EventRecord); }
	}
};

RealTimeConsumer::RealTimeConsumer(const wchar_t* sessionName)
	: m_data{ std::make_unique<Data>() }
{
	HRESULT res;
	res = CoCreateGuid(&m_data->sessionGuid);
	if (res != S_OK)
	{
		std::cerr << "Failed to generate session GUID" << std::endl;
		return;
	}

	m_data->sessionName = sessionName;

	m_data->sessionPropertiesBuffer.resize(
		sizeof(EVENT_TRACE_PROPERTIES)
		+ (m_data->sessionName.length() + 1) * sizeof(wchar_t));

	memset(m_data->sessionPropertiesBuffer.data(), 0, m_data->sessionPropertiesBuffer.size());

	m_data->sessionProperties = reinterpret_cast<EVENT_TRACE_PROPERTIES*>(m_data->sessionPropertiesBuffer.data());
	memcpy(m_data->sessionPropertiesBuffer.data() + sizeof(EVENT_TRACE_PROPERTIES), m_data->sessionName.c_str(), (m_data->sessionName.length() + 1) * sizeof(wchar_t));

	m_data->sessionProperties->Wnode.BufferSize = static_cast<ULONG>(m_data->sessionPropertiesBuffer.size());
	m_data->sessionProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
	m_data->sessionProperties->Wnode.ClientContext = 1; // QPC clock resolution
	m_data->sessionProperties->Wnode.Guid = m_data->sessionGuid;

	m_data->sessionProperties->BufferSize = 96;
	m_data->sessionProperties->MinimumBuffers = 4;
	m_data->sessionProperties->MaximumBuffers = 64;

	m_data->sessionProperties->LogFileMode = EVENT_TRACE_FILE_MODE_SEQUENTIAL | EVENT_TRACE_REAL_TIME_MODE;
	m_data->sessionProperties->LogFileNameOffset = 0; // no log file
	m_data->sessionProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

	ULONG resl = StartTraceW(&m_data->handle, m_data->sessionName.c_str(), m_data->sessionProperties);
	if (resl != ERROR_SUCCESS)
	{
		std::cerr << "Failed to start real-time trace session: " << resl << std::endl;
		return;
	}

	m_data->traceLog.LogFileName = nullptr;
	m_data->traceLog.LoggerName = const_cast<wchar_t*>(m_data->sessionName.c_str());
	m_data->traceLog.ProcessTraceMode = PROCESS_TRACE_MODE_EVENT_RECORD | PROCESS_TRACE_MODE_REAL_TIME;
	m_data->traceLog.EventRecordCallback = &Data::OnTraceEvent;
	m_data->traceLog.Context = static_cast<void*>(m_data.get());

	m_data->hprocess = OpenTraceW(&m_data->traceLog);
	if (m_data->hprocess == INVALID_PROCESSTRACE_HANDLE)
	{
		std::cerr << "Failed to open trace session: " << GetLastError() << std::endl;
		return;
	}

	m_data->hprocessWorker = std::thread{
		[hProcess = m_data->hprocess]
		{
			TRACEHANDLE hTraceProcess = hProcess;
			if (hTraceProcess == INVALID_PROCESSTRACE_HANDLE)
			{
				std::cerr << "Failed to process trace session: invalid handle" << std::endl;
				return;
			}
			std::cout << "Start ProcessTrace..." << std::endl;
			ULONG resl = ProcessTrace(&hTraceProcess, 1, nullptr, nullptr);
			std::cout << "End ProcessTrace." << std::endl;
			if (resl != ERROR_SUCCESS)
			{
				std::cerr << "Failed to process trace session: " << resl << std::endl;
			}
			CloseTrace(hTraceProcess);
		}
	};

	// Wait for ProcessTrace being started, before continuing here
	// In this trivial example, otherwise `ProcessTrace` could be called after we already reached the dtor, which closes the trace session.
	Sleep(10);
}

RealTimeConsumer::~RealTimeConsumer()
{
	if (m_data->handle != 0)
	{
		ULONG resl = ControlTraceW(m_data->handle, m_data->sessionName.c_str(), m_data->sessionProperties, EVENT_TRACE_CONTROL_STOP);
		if (resl != ERROR_SUCCESS)
		{
			std::cerr << "Failed to stop real-time session: " << resl << std::endl;
			return;
		}
	}

	if (m_data->hprocessWorker.joinable())
	{
		m_data->hprocessWorker.join();
	}
}

bool RealTimeConsumer::EnableTraceProviderCapture(GUID const& guid)
{
	ULONG resl = EnableTraceEx2(m_data->handle, &guid, EVENT_CONTROL_CODE_ENABLE_PROVIDER, TRACE_LEVEL_VERBOSE, 0, 0, 0, nullptr);
	if (resl != ERROR_SUCCESS)
	{
		std::cerr << "Failed to enable app trace event provider: " << resl << std::endl;
		return false;
	}
	return true;
}

void RealTimeConsumer::SetOnTraceEventCallback(std::function<void(PEVENT_RECORD)> func)
{
	m_data->onTraceEvent = func;
}
