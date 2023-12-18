#include "LCD.h"
#include "Out.h"
#include "Keyboard.h"
#include "Leds.h"
#include "Volume.h"
#include "IR.h"
#include "Settings.h"

#include <HID-Project.h>
#include <ArduinoSTL.h>

#include <array>
#include <map>

namespace pins
{
const int displayData = 2;
const int displayLatch = 3;
const int displayClock = 4;
} // namespace pins

LiquidCrystal lcd(pins::displayLatch, pins::displayClock, pins::displayData);
Settings settings;

void setup()
{
    if (out::Enabled) {
        out::cout.Init();
    }
    setupKeyboard();
    setupLeds();
    setupVolume();
    setupIR();

    lcd.begin();
    lcd.print(F("Hello, World!"));
    lcd.setPersistentStrings(F(""), F(""));
}

struct ScheduledFunction
{
    typedef void (*FunPtr)();
    FunPtr func;
    int interval;
    unsigned long last;
};

ScheduledFunction scheduledFuncs[] = {{&readMatrix, 1, 0}, {&checkVolume, 100, 0},
    {&blinkLed, 150, 0},/*{&displayLoop, 500, 0},*/ {&checkLocks, 100, 0}, {&checkIR, 100, 0}};

void loop()
{
    auto currTime = millis();
    for (int i = 0; i < sizeof(scheduledFuncs) / sizeof(ScheduledFunction); ++i) {
        if (currTime - scheduledFuncs[i].last > scheduledFuncs[i].interval) {
            scheduledFuncs[i].func();
            scheduledFuncs[i].last = currTime;
        }
    }
}
