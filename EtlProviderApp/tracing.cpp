#include "tracing.h"

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <TraceLoggingProvider.h>

#include <cstdint>

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

/*
* TraceLoggingEventTag
*/

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
