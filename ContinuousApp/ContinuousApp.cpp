// ContinuousApp
// Continuous rolls dice until keyboard enter tells it to stop
// Emits events on start, stop, and dice
//

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <conio.h>
#include <TraceLoggingProvider.h>

#include <array>
#include <chrono>
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
    void TraceMessage(char const* msg)
    {
        TraceLoggingWrite(g_traceloggingProvider,
            "Message",
            TraceLoggingDescription("A generic message event"),
            TraceLoggingString(msg, "message", "The text message")
        );
    }

    void TraceDie(int value)
    {
        TraceLoggingWrite(g_traceloggingProvider,
            "DieRolled",
            TraceLoggingDescription("A die has been rolled event"),
            TraceLoggingInt32(value, "value", "The value of the die")
        );
    }

    void TraceDiceStats(std::array<unsigned int, 6> const& counter)
    {
        TraceLoggingWrite(g_traceloggingProvider,
            "DiceStats",
            TraceLoggingDescription("Counters of all dice rolled until now"),
            TraceLoggingUInt32(counter[0], "D1", "Number of dice showing 1"),
            TraceLoggingUInt32(counter[1], "D2", "Number of dice showing 2"),
            TraceLoggingUInt32(counter[2], "D3", "Number of dice showing 3"),
            TraceLoggingUInt32(counter[3], "D4", "Number of dice showing 4"),
            TraceLoggingUInt32(counter[4], "D5", "Number of dice showing 5"),
            TraceLoggingUInt32(counter[5], "D6", "Number of dice showing 6")
            );
    }

}

int main()
{
    std::cout << "Rolling Dice...\n"
        << "Press any key to leave.\n" << std::endl;

    TraceMessage("Start");

    constexpr std::chrono::system_clock::duration diceInterval{ std::chrono::milliseconds(100) };
    constexpr std::chrono::system_clock::duration reportInterval{ std::chrono::seconds(1) };

    std::random_device r;
    std::default_random_engine e(r());
    std::uniform_int_distribution<int> rollDi(1, 6);

    std::array<unsigned int, 6> counter{ 0, 0, 0, 0, 0, 0 };
    std::chrono::system_clock::time_point nextReport{ std::chrono::system_clock::now() + reportInterval };

    while (!_kbhit())
    {
        int dieValue = rollDi(e);
        TraceDie(dieValue);

        counter[dieValue - 1]++;

        Sleep(static_cast<DWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(diceInterval).count()));

        const auto now = std::chrono::system_clock::now();
        if (now >= nextReport)
        {
            nextReport = now + reportInterval;

            std::cout << "Dice stats."
                << "  1: " << counter[0]
                << "  2: " << counter[1]
                << "  3: " << counter[2]
                << "  4: " << counter[3]
                << "  5: " << counter[4]
                << "  6: " << counter[5]
                << std::endl;

            TraceDiceStats(counter);
        }
    }

    std::cout << "\nbye." << std::endl;

    TraceMessage("End");

    return 0;
}
