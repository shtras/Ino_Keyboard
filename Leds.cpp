#include "Leds.h"
#include "Settings.h"

#include <Arduino.h>
#include <array>

namespace
{
namespace pins
{
const int ledsClock = 7;
const int ledsLatch = 8;
const int ledsData = 9;
} // namespace pins

std::array<int, 8> ledAnimation = {0};
int ledAnimationCounter = 0;
int ledAnimationLength = 0;
int ledAnimationTimeout = 0;

int persistentLedValue = 0;
} // namespace

unsigned long ledTimeout = 0;
unsigned long randomTimeout = 0;

void setupLeds()
{
    pinMode(pins::ledsClock, OUTPUT);
    pinMode(pins::ledsData, OUTPUT);
    pinMode(pins::ledsLatch, OUTPUT);
}

void setLedAnimation(std::array<int, 8> animation, int length, int timeout)
{
    if (!settings.ledsEnabled) {
        return;
    }
    length = std::min(length, static_cast<int>(ledAnimation.size()));
    for (int i = 0; i < length; ++i) {
        ledAnimation[i] = animation[i];
    }
    ledAnimationLength = length;
    ledAnimationCounter = 0;
    ledAnimationTimeout = timeout;
}

void setLeds(int value, int timeout)
{
    if (!settings.ledsEnabled) {
        value = 0;
    }
    pinMode(pins::ledsClock, OUTPUT);
    pinMode(pins::ledsData, OUTPUT);
    pinMode(pins::ledsLatch, OUTPUT);

    digitalWrite(pins::ledsLatch, LOW);
    shiftOut(pins::ledsData, pins::ledsClock, LSBFIRST, value);
    digitalWrite(pins::ledsLatch, HIGH);
    if (timeout) {
        ledTimeout = millis() + timeout;
    } else {
        persistentLedValue = value;
        ledTimeout = 0;
    }
}

void blinkLed()
{
    if (ledAnimationTimeout) {
        if (ledAnimationCounter >= ledAnimationLength) {
            ledAnimationTimeout = 0;
            return;
        }
        setLeds(ledAnimation[ledAnimationCounter++], ledAnimationTimeout);
        return;
    }
    if (ledTimeout == 0 && settings.randomLeds && randomTimeout < millis()) {
        setLeds(rand() % 255, 0);
        randomTimeout = millis() + 1000;
    } else if (ledTimeout && ledTimeout < millis()) {
        setLeds(persistentLedValue, 0);
    }
}
