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

std::map<int, KeyboardKeycode> buttonMapping = {{17, KEY_0}, {3, KEYPAD_DOT}, {16, KEYPAD_1},
    {18, KEYPAD_2}, {19, KEYPAD_3}, {15, KEYPAD_4}, {11, KEYPAD_5}, {7, KEYPAD_6}, {14, KEYPAD_7},
    {10, KEYPAD_8}, {6, KEYPAD_9}, {2, KEYPAD_ENTER}, {1, KEYPAD_ADD}, {0, KEYPAD_SUBTRACT},
    {9, KEYPAD_DIVIDE}, {5, KEYPAD_MULTIPLY}};

std::map<int, ConsumerKeycode> consumerMapping = {{13, CONSUMER_CALCULATOR}};

namespace pins
{
const int potentiometer = A0;
const int joyX = A1;
const int joyY = A2;
const int joyBtn = 15;
} // namespace pins

void setup()
{
    if (out::Enabled) {
        out::cout.Init();
    }

    for (auto row : rows) {
        out::cout << row << F(" as input");
        pinMode(row, INPUT);
    }

    for (auto col : cols) {
        out::cout << col << F(" as input-pullup");
        pinMode(col, INPUT_PULLUP);
    }
    for (auto& btn : buttonState) {
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
    out::cout << F("Pressed ") << idx << out::endl;
    if (buttonMapping.count(idx) > 0) {
        auto key = buttonMapping[idx];
        out::cout << F("Sending ") << key << F(" ") << KeyboardKeycode((uint8_t)(key & 0xFF))
                  << out::endl;
        Keyboard.press(key);
    } else if (consumerMapping.count(idx) > 0) {
        Consumer.write(consumerMapping[idx]);
    }
}

void uname()
{
    Keyboard.print("");
}

void pwd()
{
    Keyboard.print("");
}

void onKeyUp(int idx)
{
    out::cout << F("Released ") << idx << out::endl;
    if (buttonMapping.count(idx) > 0) {
        Keyboard.release(buttonMapping[idx]);
    }
    if (idx == 4) {
        //out::Enabled = !out::Enabled;
        //if (out::Enabled) {
        //    out::cout.Init();
        //}
        Consumer.press(MEDIA_VOLUME_MUTE);
        delay(10);
        Consumer.release(MEDIA_VOLUME_MUTE);
    } else if (idx == 8) {
        pwd();
    } else if (idx == 12) {
        uname();
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
    for (auto col : cols) {
        // col: set to output to low
        pinMode(col, OUTPUT);
        digitalWrite(col, LOW);

        // row: interate through the rows
        for (auto row : rows) {
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
    for (auto row : rows) {
        if (row < 10) {
            out::cout << F("0");
        }
        out::cout << row << F(": ");

        for (auto col : cols) {
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

    int joyX = analogRead(pins::joyX);
    int joyY = analogRead(pins::joyY);
    double dJoyX = pow((joyX - 498) / 512.0, 1);
    double dJoyY = pow((joyY - 498) / 512.0, 1);
    if (fabs(dJoyX) < 0.01 && fabs(dJoyY) < 0.01) {
        return;
    }
    dMouseX += dJoyX * -10.0;
    dMouseY += dJoyY * 10.0;
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
}

void checkVolume()
{
    int val = abs(1024 - analogRead(pins::potentiometer));
    double targetVolume = val / 1024.0;
    double deltaVolume = volume - targetVolume;
    int steps = fabs(deltaVolume) / 0.02;
    while (fabs(volume - targetVolume) > 0.04) {
        out::cout << F("From ") << volume << F(" To ") << targetVolume;
        if (volume < targetVolume) {
            Consumer.press(MEDIA_VOLUME_UP);
            delay(10);
            Consumer.release(MEDIA_VOLUME_UP);
            downs = 0;
            ++ups;
            out::cout << F(" Up ") << ups << out::endl;
            volume += 0.02;
            delay(10);
        } else {
            Consumer.press(MEDIA_VOLUME_DOWN);
            delay(10);
            Consumer.release(MEDIA_VOLUME_DOWN);
            ups = 0;
            ++downs;
            out::cout << F(" Down ") << downs << out::endl;
            volume -= 0.02;
            delay(10);
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

ScheduledFunction scheduledFuncs[] = {{&readMatrix, 1, 0}, {&checkVolume, 100, 0},
    {&blinkLed, 1000, 0}, {&printMatrix, 100, 0}, {&checkMouse, 10, 0}};

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
