#pragma once
#include <ArduinoSTL.h>
#include <array>

extern unsigned long ledTimeout;

extern "C" {
    void setupLeds();
    void setLeds(int value, int timeout);
    void setLedAnimation(std::array<int, 8> animation, int length, int timeout);
    void blinkLed();
}
