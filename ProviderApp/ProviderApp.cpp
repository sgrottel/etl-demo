//
// ProviderApp.cpp
//
// Copyright(c) 2023 Sebastian Grottel
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#include "tracing.h"
#include "RealTimeConsumer.h"

#include <iostream>

int main()
{
    RealTimeConsumer consumer{ L"EtlDemoProviderAppTraceSession" };
    consumer.EnableTraceProviderCapture({ 0x67272c9a, 0x6d34, 0x579c, 0x91, 0x36, 0xc7, 0xaf, 0xf6, 0xf4, 0xb0, 0x0e });
    consumer.SetOnTraceEventCallback([](auto _) { std::cout << "Trace Event !" << std::endl; });

    std::cout << "Setup\n";
    tracing::Setup();

    std::cout << "Here we go\n";

    tracing::WriteEventInts(1, 15);
    tracing::WriteEventInts2(2, 27);
    tracing::WriteEventIntAndStr(3, "woof?");
    tracing::WriteEventInts3(4, 42);
    tracing::WriteEventIntAndStr(5, "woof!");
    tracing::WriteEventInts(6, 666);
    tracing::WriteEventIntsEvil(7, 79);
    tracing::WriteEventInts(8, 8015);

    std::cout << "Teardown\n";
    tracing::Teardown();

    std::cout << "Done.\n";
}
