#include "MyTraceEventObserver.h"

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <evntcons.h>
#include <tdh.h>

#include <iostream>
#include <vector>
#include <cassert>

namespace
{
	template<typename T>
	void DecodeAndPrintPropertyAs(EVENT_PROPERTY_INFO const& propInfo, uint8_t*& data, uint32_t& dataLen)
	{
		assert(propInfo.length == sizeof(T));
		if (dataLen >= propInfo.length)
		{
			// parse value
			std::cout << *reinterpret_cast<T*>(data);

			data += propInfo.length;
			dataLen -= propInfo.length;
		}
		else
		{
			std::cout << "(error)";
		}
	}

}

void MyTraceEventObserver(PEVENT_RECORD er)
{
	static constexpr GUID providerId{ 0x67272c9a, 0x6d34, 0x579c, 0x91, 0x36, 0xc7, 0xaf, 0xf6, 0xf4, 0xb0, 0x0e };
	if (!IsEqualGUID(er->EventHeader.ProviderId, providerId)) return;

	ULONG bufSize = 0;
	TDHSTATUS res = TdhGetEventInformation(er, 0, nullptr, nullptr, &bufSize);
	if (res != ERROR_SUCCESS && res != ERROR_INSUFFICIENT_BUFFER)
	{
		std::cerr << "Failed to receive required buffer size from 'GetEventInformation': " << res << std::endl;
		return;
	}
	if (bufSize == 0)
	{
		std::cerr << "Failed to receive required buffer size from 'GetEventInformation': zero" << std::endl;
		return;
	}

	std::vector<uint8_t> buf;
	buf.resize(bufSize);
	TRACE_EVENT_INFO* teInfo = reinterpret_cast<PTRACE_EVENT_INFO>(buf.data());
	res = TdhGetEventInformation(er, 0, nullptr, teInfo, &bufSize);
	if (res != ERROR_SUCCESS)
	{
		std::cerr << "Failed to receive 'GetEventInformation': " << res << std::endl;
		return;
	}

	const wchar_t* eventName = reinterpret_cast<wchar_t*>(buf.data() + teInfo->EventNameOffset);
	if (std::wstring(L"WriteEventInts") != eventName)
	{
		return;
	}

	std::cout << "Received `WriteEventInts`:";

	uint8_t* data = static_cast<uint8_t*>(er->UserData);
	uint32_t dataLen = er->UserDataLength;

	unsigned int propertyCount = teInfo->PropertyCount;
	for (unsigned int pIdx = 0; pIdx < propertyCount; ++pIdx)
	{
		auto const& propInfo = teInfo->EventPropertyInfoArray[pIdx];
		const wchar_t* propName = reinterpret_cast<wchar_t*>(buf.data() + propInfo.NameOffset);
		std::wcout << L" " << propName << L" = ";

		// this is super-simple decoding, not supporting structs, arrays, and special flags
		// data decoding breaks after on unknown property is encountered.
		if ((propInfo.Flags & PropertyStruct) == PropertyStruct)
		{
			std::cout << "{?}";
			data = nullptr; // no longer able to decode user data
			dataLen = 0;
		}
		else
		{
			switch (propInfo.nonStructType.InType)
			{
			case TDH_OUTTYPE_INT:
				DecodeAndPrintPropertyAs<int32_t>(propInfo, data, dataLen);
				break;
			case TDH_OUTTYPE_FLOAT:
				DecodeAndPrintPropertyAs<float>(propInfo, data, dataLen);
				break;
			case TDH_OUTTYPE_LONG:
				DecodeAndPrintPropertyAs<int64_t>(propInfo, data, dataLen);
				break;
			default:
				std::cout << "?";
				data = nullptr; // no longer able to decode user data
				dataLen = 0;
				break;
			}
		}
		std::cout << ";";
	}

	std::cout << "\n";
}
