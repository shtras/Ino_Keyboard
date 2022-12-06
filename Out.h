#pragma once
#include "HID-Project.h"

//#define USE_SERIAL

namespace out
{
class Endl
{
};
extern Endl endl;

class Cout
{
public:
    void Init()
    {
#ifdef USE_SERIAL
        Serial.begin(115200);
        while (!Serial)
            ; // Wait untilSerial is ready - Leonardo
#endif
    }
    Cout& operator<<(Endl&)
    {
#ifdef USE_SERIAL
        Serial.println();
#endif
        return *this;
    }
    template <typename T>
    Cout& operator<<(const T& t)
    {
#ifdef USE_SERIAL
        Serial.print(t);
#endif
        return *this;
    }
};
extern Cout cout;
} // namespace out