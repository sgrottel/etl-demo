#include "tracing.h"

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <TraceLoggingProvider.h>

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

TRACELOGGING_DECLARE_PROVIDER(g_traceloggingProvider);

TRACELOGGING_DEFINE_PROVIDER(g_traceloggingProvider,
    "EtlProviderApp",
    (0x67272c9a, 0x6d34, 0x579c, 0x91, 0x36, 0xc7, 0xaf, 0xf6, 0xf4, 0xb0, 0x0e));

void tracing::Setup()
{
    TraceLoggingRegister(g_traceloggingProvider);
}

void tracing::Teardown()
{
    TraceLoggingUnregister(g_traceloggingProvider);
}

void tracing::WriteEventInts(
    int i1,
    int i2)
{
    TraceLoggingWrite(g_traceloggingProvider,
        "WriteEventInts",
        TraceLoggingDescription("The WriteEventInts Event"),
        TraceLoggingInt32(i1, "i1", "The first payload integer"),
        TraceLoggingInt32(i2, "i2", "The second payload integer")
    );
}

void tracing::WriteEventInts2(
    int i1,
    int i2)
{
    TraceLoggingWrite(g_traceloggingProvider,
        "WriteEventInts",
        TraceLoggingDescription("The WriteEventInts Event"),
        TraceLoggingInt32(i1, "i1", "The first payload integer"),
        TraceLoggingInt32(i2, "i2", "The second payload integer")
    );
}

void tracing::WriteEventInts3(
    int i1,
    int i2)
{
    // This function basically does the same thing as:
    //
    // TraceLoggingWrite(g_traceloggingProvider,
    //     "WriteEventInts",
    //     TraceLoggingInt32(i1, "i1", "The first payload integer"),
    //     TraceLoggingInt32(i2, "i2", "The second payload integer")
    // );
    //
    // BUT
    // Additionally sets the `Task` field in the event meta data to 1,
    // to mark this event being part of an application specific grouping.

    EVENT_DESCRIPTOR metadata;
    ZeroMemory(&metadata, sizeof(EVENT_DESCRIPTOR));
    metadata.Id = 0;
    metadata.Version = 0;
    metadata.Channel = 11; // TraceLogging-based
    metadata.Level = 5; // verbose
    metadata.Opcode = 0;
    metadata.Task = 1; // app-specific grouping <== this is important for this test case
    metadata.Keyword = 0;

    constexpr int fieldDataSize = 4;
    EVENT_DATA_DESCRIPTOR fieldData[fieldDataSize];

    fieldData[0].Ptr = reinterpret_cast<ULONGLONG>(g_traceloggingProvider->ProviderMetadataPtr);
    fieldData[0].Size = *g_traceloggingProvider->ProviderMetadataPtr;
    fieldData[0].Reserved = 2; // = EVENT_DATA_DESCRIPTOR_TYPE_PROVIDER_METADATA

    constexpr char eventName[] = "WriteEventInts";
    constexpr char i1Name[] = "i1";
    constexpr char i2Name[] = "i2";

    std::vector<uint8_t> eventMetadata;
    eventMetadata.resize(
        sizeof(uint16_t) // data size
        + 1 // event tag type
        + sizeof(eventName)
        + sizeof(i1Name)
        + 1 // payload value type
        + sizeof(i2Name)
        + 1 // payload value type
    );
    uint8_t *ptr = eventMetadata.data();

    *reinterpret_cast<uint16_t*>(ptr) = static_cast<uint16_t>(eventMetadata.size());
    ptr += 2;
    *ptr = 0;
    ptr += 1;
    memcpy(ptr, eventName, sizeof(eventName));
    ptr += sizeof(eventName);
    memcpy(ptr, i1Name, sizeof(i1Name));
    ptr += sizeof(i1Name);
    *ptr = 7; // = TlgInINT32
    ptr += 1;
    memcpy(ptr, i2Name, sizeof(i2Name));
    ptr += sizeof(i2Name);
    *ptr = 7; // = TlgInINT32
    ptr += 1;
    assert(ptr == eventMetadata.data() + eventMetadata.size());

    fieldData[1].Ptr = reinterpret_cast<ULONGLONG>(eventMetadata.data());
    fieldData[1].Size = static_cast<ULONG>(eventMetadata.size());
    fieldData[1].Reserved = 1; // = EVENT_DATA_DESCRIPTOR_TYPE_EVENT_METADATA

    EventDataDescCreate(fieldData + 2, &i1, 4);
    EventDataDescCreate(fieldData + 3, &i2, 4);

    ULONG retCode = EventWriteTransfer(
        g_traceloggingProvider->RegHandle,
        &metadata,
        NULL,
        NULL,
        fieldDataSize,
        fieldData
        );

    if (retCode != ERROR_SUCCESS)
    {
        std::cout << "WriteEventInts3.EventWriteTransfer failed with " << retCode << "\n";
    }
}

void tracing::WriteEventIntsEvil(
    int i1,
    int i2)
{
    // emits the same name as `WriteEventInts` but with other fields!
    float f = 4.2f;
    TraceLoggingWrite(g_traceloggingProvider,
        "WriteEventInts",
        TraceLoggingDescription("The WriteEventInts Event"),
        TraceLoggingFloat32(f, "f", "An extra payload float"),
        TraceLoggingInt64(static_cast<int64_t>(i1), "i1", "The first payload integer"),
        TraceLoggingInt64(static_cast<int64_t>(i2), "i2", "The second payload integer")
    );
}

void tracing::WriteEventIntAndStr(
    int i1,
    char const* str)
{
    TraceLoggingWrite(g_traceloggingProvider,
        "WriteEventIntAndStr",
        TraceLoggingDescription("The WriteEventIntAndStr Event"),
        TraceLoggingInt32(i1, "i1", "The payload integer"),
        TraceLoggingString(str, "str", "The payload string")
    );
}
