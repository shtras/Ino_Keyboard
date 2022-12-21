#include "HID-Project.h"

#include <ArduinoSTL.h>

#include "Out.h"
#include <array>
#include <map>

constexpr int numCols = 5;
constexpr int numRows = 4;
std::array<int, numCols> cols = {2, 3, 4, 5, 10};
std::array<int, numRows> rows = {6, 7, 8, 9};
std::array<byte, numCols * numRows> buttonState;

std::map<int, KeyboardKeycode> buttonMapping = {
    {17, KEY_0},
    {3, KEYPAD_DOT},
    {16, KEYPAD_1},
    {18, KEYPAD_2},
    {19, KEYPAD_3}
};

namespace pins
{
const int potentiometer = A0;
const int joyX = A2;
const int joyY = A1;
const int joyBtn = 15;
} // namespace pins

void setup()
{
    out::cout.Init();

    for (auto row: rows) {
        out::cout << row << " as input";
        pinMode(row, INPUT);
    }

    for (auto col: cols) {
        out::cout << col << " as input-pullup";
        pinMode(col, INPUT_PULLUP);
    }
    for (auto& btn: buttonState) {
        btn = HIGH;
    }
    pinMode(pins::potentiometer, INPUT);
    pinMode(pins::joyBtn, INPUT_PULLUP);
    pinMode(pins::joyX, INPUT);
    pinMode(pins::joyY, INPUT);
    Mouse.begin();
    Keyboard.begin();
    Consumer.begin();
}

void onKeyDown(int idx)
{
    out::cout << "Pressed " << idx << out::endl;
    if (buttonMapping.count(idx) > 0) {
        auto key = buttonMapping[idx];
        out::cout << "Sending " << key << " " << KeyboardKeycode((uint8_t)(key & 0xFF)) << out::endl;
        Keyboard.press(key);
    }
}

void onKeyUp(int idx)
{
    out::cout << "Released " << idx << out::endl;
    if (buttonMapping.count(idx) > 0) {
        Keyboard.release(buttonMapping[idx]);
    }
}

void processButton(int idx, int value)
{
    if (buttonState.at(idx) == value) {
        return;
    }
    buttonState[idx] = value;
    if (value == LOW) {
        onKeyDown(idx);
    } else {
        onKeyUp(idx);
    }
}

void readMatrix()
{
    int buttonIdx = 0;
    // iterate the columns
    for (auto col: cols) {
        // col: set to output to low
        pinMode(col, OUTPUT);
        digitalWrite(col, LOW);

        // row: interate through the rows
        for (auto row: rows) {
            pinMode(row, INPUT_PULLUP);
            processButton(buttonIdx++, digitalRead(row));
            pinMode(row, INPUT);
        }
        // disable the column
        pinMode(col, INPUT);
    }
}

void printMatrix()
{
    return;
    int buttonIdx = 0;
    for (auto row: rows) {
        if (row < 10) {
            out::cout << F("0");
        }
        out::cout << row << F(": ");

        for (auto col: cols) {
            out::cout << buttonState.at(buttonIdx++) << F(", ");
        }
        out::cout << out::endl;
    }
    out::cout << out::endl;
}

int lastVal = 1024;
double volume = 1.0;
int ups = 0;
int downs = 0;
double dMouseX = 0;
double dMouseY = 0;
unsigned long mouseX = 0;
unsigned long mouseY = 0;
bool mouseClicked = false;

void checkMouse()
{
    int joyX = analogRead(pins::joyX);
    int joyY = analogRead(pins::joyY);
    double dJoyX = pow((joyX - 500) / 512.0, 3);
    double dJoyY = pow((joyY - 500) / 512.0, 3);
    dMouseX += dJoyX * 10.0;
    dMouseY += dJoyY * 10.0;
    if (fabs(dJoyX) > 0.01 || fabs(dJoyY) > 0.01) {
        out::cout << joyX << "(" << dJoyX << "), " << joyY << "(" << dJoyY << ") "
                  << digitalRead(pins::joyBtn) << out::endl;
    }
    if (static_cast<decltype(mouseX)>(dMouseX) != mouseX) {
        int offset = static_cast<decltype(mouseX)>(dMouseX) - mouseX;
        Mouse.move(-offset, 0);
        mouseX = static_cast<decltype(mouseX)>(dMouseX);
    }
    if (static_cast<decltype(mouseY)>(dMouseY) != mouseY) {
        int offset = static_cast<decltype(mouseY)>(dMouseY) - mouseY;
        Mouse.move(0, -offset);
        mouseY = static_cast<decltype(mouseY)>(dMouseY);
    }
    if (digitalRead(pins::joyBtn) == LOW) {
        if (!mouseClicked) {
            Mouse.press();
            mouseClicked = true;
        }
    } else {
        if (mouseClicked) {
            Mouse.release();
            mouseClicked = false;
        }
    }
}

void checkVolume()
{
    int val = analogRead(pins::potentiometer);
    double targetVolume = val / 1024.0;
    double deltaVolume = volume - targetVolume;
    int steps = fabs(deltaVolume) / 0.02;
    while (fabs(volume - targetVolume) > 0.04) {
        out::cout << "From " << volume << " To " << targetVolume;
        if (volume < targetVolume) {
            Consumer.write(MEDIA_VOLUME_UP);
            downs = 0;
            ++ups;
            out::cout << " Up " << ups << out::endl;
            volume += 0.02;
            delay(5);
        } else {
            Consumer.write(MEDIA_VOLUME_DOWN);
            ups = 0;
            ++downs;
            out::cout << " Down " << downs << out::endl;
            volume -= 0.02;
            delay(5);
        }
    }
}

void blinkLed()
{
#if 0
    static bool lit = false;
    if (lit) {
        digitalWrite(LED_BUILTIN_RX, HIGH);
        digitalWrite(LED_BUILTIN_TX, LOW);
    } else {
        //digitalWrite(LED_BUILTIN, HIGH);
        digitalWrite(LED_BUILTIN_RX, LOW);
        digitalWrite(LED_BUILTIN_TX, HIGH);
    }
    lit = !lit;
#endif
}

struct ScheduledFunction
{
    typedef void (*FunPtr)();
    FunPtr func;
    int interval;
    unsigned long last;
};

ScheduledFunction scheduledFuncs[] = {
    {&checkVolume, 100, 0}, {&blinkLed, 1000, 0}, {&printMatrix, 100, 0}};

void loop()
{
    auto currTime = millis();
    for (int i = 0; i < sizeof(scheduledFuncs) / sizeof(ScheduledFunction); ++i) {
        if (currTime - scheduledFuncs[i].last > scheduledFuncs[i].interval) {
            scheduledFuncs[i].func();
            scheduledFuncs[i].last = currTime;
        }
    }
    readMatrix();
}
