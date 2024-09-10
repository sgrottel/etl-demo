// MinimalApp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TraceLoggingProvider.h>
#include <iostream>
#include <random>

// declare can happen in a header, if you need this provider variable in multiple build units
TRACELOGGING_DECLARE_PROVIDER(g_traceloggingProvider);

// definition must be in exactly one build unit
TRACELOGGING_DEFINE_PROVIDER(g_traceloggingProvider,
    "EtlProviderApp",
    (0x67272c9a, 0x6d34, 0x579c, 0x91, 0x36, 0xc7, 0xaf, 0xf6, 0xf4, 0xb0, 0x0e));

namespace
{

    // It might make sense to wrap the trace-event-write call in a utility function (trivially inlineable by the compiler) for cleaner code
    void WriteEventInts(
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

}

int main()
{
    TraceLoggingRegister(g_traceloggingProvider);

    std::random_device r;
    std::default_random_engine e(r());
    std::uniform_int_distribution<int> uniform_dist(1, 6);

    std::cout << "I will now roll 10x 2x D6 dice:\n";

    for (int round = 0; round < 10; round++)
    {
        int v1 = uniform_dist(e);
        int v2 = uniform_dist(e);
        WriteEventInts(v1, v2);

        std::cout << "\t" << round << ": [" << v1 << "] [" << v2 << "]" << std::endl;

        Sleep(100);
    }
    std::cout << "done." << std::endl;

    TraceLoggingUnregister(g_traceloggingProvider);
    return 0;
}
