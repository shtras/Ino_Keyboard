#include "Keyboard.h"
#include "Out.h"
#include "Leds.h"

#include <Arduino.h>
#include <MultiReport/Consumer.h>
#include <MultiReport/ImprovedKeyboard.h>
#include <SingleReport/BootKeyboard.h>

#include <map>
#include <array>

namespace
{
constexpr int numRows = 5;
constexpr int numCols = 4;
std::array<int, numRows> rows = {10, 6, 5, 15, 14};
std::array<int, numCols> cols = {21, 20, 19, 18};
std::array<byte, numRows * numCols> buttonState;

std::map<int, ConsumerKeycode> consumerMapping = {{13, CONSUMER_CALCULATOR}};
std::map<int, KeyboardKeycode> buttonMapping = {{17, KEY_0}, {3, KEYPAD_DOT}, {16, KEYPAD_1},
    {18, KEYPAD_2}, {19, KEYPAD_3}, {15, KEYPAD_4}, {11, KEYPAD_5}, {7, KEYPAD_6}, {14, KEYPAD_7},
    {10, KEYPAD_8}, {6, KEYPAD_9}, {2, KEYPAD_ENTER}, {1, KEYPAD_ADD}, {0, KEYPAD_SUBTRACT},
    {9, KEYPAD_DIVIDE}, {5, KEYPAD_MULTIPLY}};

void onKeyDown(int idx)
{
    out::cout << F("Pressed ") << idx << out::endl;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Key down: "));
    lcd.print(idx);
    if (buttonMapping.count(idx) > 0) {
        auto key = buttonMapping[idx];
        out::cout << F("Sending ") << key << F(" ") << KeyboardKeycode((uint8_t)(key & 0xFF))
                  << out::endl;
        //Keyboard.press(key);
    } else if (consumerMapping.count(idx) > 0) {
        //Consumer.write(consumerMapping[idx]);
    }
}

void uname()
{
    Keyboard.print(F(""));
}

void pwd()
{
    Keyboard.print(F(""));
}

void onKeyUp(int idx)
{
    out::cout << F("Released ") << idx << out::endl;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Key up: "));
    lcd.print(idx);

    if (buttonMapping.count(idx) > 0) {
        //Keyboard.release(buttonMapping[idx]);
    }
    if (idx == 8) {
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
} // namespace

void setupKeyboard()
{
    for (auto row : cols) {
        out::cout << row << F(" as input");
        pinMode(row, INPUT);
    }

    for (auto col : rows) {
        out::cout << col << F(" as input-pullup");
        pinMode(col, INPUT_PULLUP);
    }
    for (auto& btn : buttonState) {
        btn = HIGH;
    }

    Keyboard.begin();
    Consumer.begin();
    BootKeyboard.begin();
}

void readMatrix()
{
    int buttonIdx = 0;
    // iterate the rows
    for (auto col : rows) {
        // col: set to output to low
        pinMode(col, OUTPUT);
        digitalWrite(col, LOW);

        // row: interate through the cols
        for (auto row : cols) {
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

void checkLocks()
{
    int lockLeds = 0;
    if (BootKeyboard.getLeds() & LED_CAPS_LOCK) {
        lockLeds |= 2;
    }
    if (!(BootKeyboard.getLeds() & LED_NUM_LOCK)) {
        lockLeds |= 4;
    }
    if (BootKeyboard.getLeds() & LED_SCROLL_LOCK) {
        lockLeds |= 1;
    }
    if (ledTimeout == 0) {
        setLeds(lockLeds, 0);
    }
}
