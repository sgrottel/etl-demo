#pragma once

namespace tracing
{

    void Setup();
    void Teardown();

    void WriteEventInts(int i1, int i2);
    void WriteEventInts2(int i1, int i2);
    void WriteEventInts3(int i1, int i2);
    void WriteEventIntsEvil(int i1, int i2);

    void WriteEventIntAndStr(int i1, char const* str);

}
