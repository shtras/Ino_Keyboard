#pragma once
#include "HID-Project.h"

//#define USE_SERIAL

namespace out
{
enum class Mode { Hex, Dec };
class Endl
{
};
extern Endl endl;
class Hex
{
};
extern Hex hex;
class Dec
{
};
extern Dec dec;

extern bool Enabled;
extern Mode mode;

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
        mode = Mode::Dec;
        if (Enabled) {
            Serial.println();
        }
#endif
        return *this;
    }
    Cout& operator<<(Hex&)
    {
#ifdef USE_SERIAL
        mode = Mode::Hex;
        return *this;
#endif
    }
    Cout& operator<<(Dec&)
    {
#ifdef USE_SERIAL
        mode = Mode::Dec;
#endif
        return *this;
    }
    template <typename T>
    Cout& operator<<(const T& t)
    {
#ifdef USE_SERIAL
        if (Enabled) {
            Serial.print(t);
        }
#endif
        return *this;
    }
    Cout& operator<<(const int& t)
    {
#ifdef USE_SERIAL
        print(t);
#endif
        return *this;
    }
    Cout& operator<<(const uint16_t& t)
    {
#ifdef USE_SERIAL
        print(t);
#endif
        return *this;
    }

private:
    template <typename T>
    void print(const T& t)
    {
#ifdef USE_SERIAL
        if (Enabled) {
            switch (mode) {
                case Mode::Hex:
                    Serial.print(t, HEX);
                    break;
                case Mode::Dec:
                    Serial.print(t);
                    break;
            }
        }
#endif
    }
};
extern Cout cout;
} // namespace out
