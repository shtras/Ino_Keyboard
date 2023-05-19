#include "Volume.h"
#include "Out.h"
#include "Leds.h"
#include "LCD.h"
#include <Arduino.h>
#include <MultiReport/Consumer.h>

namespace
{
namespace pins
{
const int encoderA = 1;
const int encoderB = 0;
} // namespace pins

double volumeValue = 0.0;
volatile double targetVolume = 0.0;
double minVolume = 0.0;
int ups = 0;
int downs = 0;

} // namespace

void encoderISR();

void setupVolume()
{
    pinMode(pins::encoderA, INPUT);
    pinMode(pins::encoderB, INPUT);
    digitalWrite(pins::encoderA, HIGH);
    digitalWrite(pins::encoderB, HIGH);
    attachInterrupt(digitalPinToInterrupt(pins::encoderA), encoderISR, CHANGE);
}

void changeVolume(ConsumerKeycode val)
{
    Consumer.press(val);
    delay(10);
    Consumer.release(val);
    delay(10);
}

void checkVolume()
{
    double deltaVolume = volumeValue - targetVolume;
    int steps = fabs(deltaVolume) / 0.02;
    while (fabs(volumeValue - targetVolume) > 0.01) {
        out::cout << F("From ") << volumeValue << F(" To ") << targetVolume;
        if (volumeValue < targetVolume) {
            changeVolume(MEDIA_VOLUME_UP);
            downs = 0;
            ++ups;
            out::cout << F(" Up ") << ups << out::endl;
            volumeValue += 0.02;
            delay(10);
        } else {
            changeVolume(MEDIA_VOLUME_DOWN);
            ups = 0;
            ++downs;
            out::cout << F(" Down ") << downs << out::endl;
            volumeValue -= 0.02;
            delay(10);
        }
        if (volumeValue > minVolume + 1.0) {
            minVolume = volumeValue - 1.0;
        }
        if (volumeValue < minVolume) {
            minVolume = volumeValue;
        }
        int ledsValue = 0;
        int numLeds = sqrt((volumeValue - minVolume)) * 8.0f;
        for (int i = 0; i < numLeds; ++i) {
            ledsValue |= 1 << i;
        }
        setLeds(ledsValue, 3000);

        int plainVolume = static_cast<int>((volumeValue - minVolume) * 100.0);
        lcd.clear();
        lcd.setCursor(0, 0);
        for (int i=0; i<plainVolume / 6.25;++i) {
            lcd.print((char)255);
        }
        lcd.setCursor(0, 1);
        lcd.print("Vol: ");
        lcd.print(plainVolume);
    }
}

volatile int lastEncoderA = 0;
void encoderISR()
{
    noInterrupts();
    int A = digitalRead(pins::encoderA);
    int B = digitalRead(pins::encoderB);
    if (A != 0 && A != lastEncoderA) {
        if (A == B) {
            targetVolume += 0.02;
            out::cout << F("CW") << out::endl;
        } else {
            targetVolume -= 0.02;
            out::cout << F("CCW") << out::endl;
        }
    }
    lastEncoderA = A;
    interrupts();
}

void addTargetVolume(float delta)
{
    targetVolume += delta;
}