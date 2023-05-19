#include "Keyboard.h"
#include "Out.h"
#include "Leds.h"
#include "Settings.h"

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
constexpr int fnBtn = 4;

typedef void (*BtnFunc)();

std::map<int, KeyboardKeycode> buttonMapping = {{5, KEY_INSERT}, {6, KEY_HOME}, {7, KEY_PAGE_UP},
    {8, KEY_LEFT_ALT}, {9, KEY_DELETE}, {10, KEY_END}, {11, KEY_PAGE_DOWN}, {12, KEY_LEFT_SHIFT},
    {14, KEY_UP_ARROW}, {16, KEY_LEFT_CTRL}, {17, KEY_LEFT_ARROW}, {18, KEY_DOWN_ARROW},
    {19, KEY_RIGHT_ARROW}};

void uname()
{
    Keyboard.print(F(""));
}

void pwd()
{
    Keyboard.print(F(""));
}

void mute()
{
    Consumer.write(MEDIA_VOL_MUTE);
    setLedAnimation({1, 9, 73, 219, 255, 0, 0, 0}, 5, 100);
}

std::map<int, BtnFunc> consumerMapping = {{0, &mute}};

void onKeyDown(int idx)
{
    out::cout << F("Pressed ") << idx << out::endl;
    if (buttonState.at(fnBtn) == LOW) {
        switch (idx) {
            case 1:
                settings.keyPressEcho = !settings.keyPressEcho;
                lcd.cprint(F("Key echo: "));
                lcd.print(settings.keyPressEcho ? F("ON") : F("OFF"));
                break;
            case 2:
                settings.irDebug = !settings.irDebug;
                lcd.cprint(F("IR debug: "));
                lcd.print(settings.irDebug ? F("ON") : F("OFF"));
                break;
            case 3:
                settings.ledsEnabled = !settings.ledsEnabled;
                lcd.cprint(F("LEDs: "));
                lcd.print(settings.ledsEnabled ? F("ON") : F("OFF"));
                setLeds(0, 0);
                break;
            case 5:
                settings.lcdEnabled = !settings.lcdEnabled;
                if (settings.lcdEnabled) {
                    lcd.cprint(F("LCD: ON"));
                } else {
                    lcd.clear();
                }
                break;
            case 6:
                settings.irEnabled = !settings.irEnabled;
                lcd.cprint(F("IR: "));
                lcd.print(settings.irEnabled ? F("ON") : F("OFF"));
                break;
        }
        return;
    }

    if (settings.keyPressEcho) {
        lcd.cprint(F("Key down: "));
        lcd.print(idx);
    }

    if (buttonMapping.count(idx) > 0) {
        auto key = buttonMapping[idx];
        out::cout << F("Sending ") << key << F(" ") << KeyboardKeycode((uint8_t)(key & 0xFF))
                  << out::endl;
        Keyboard.press(key);
    } else if (consumerMapping.count(idx) > 0) {
        consumerMapping[idx]();
    }
}

void onKeyUp(int idx)
{
    out::cout << F("Released ") << idx << out::endl;

    if (buttonState.at(fnBtn) == LOW) {
        return;
    }

    if (settings.keyPressEcho) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("Key up: "));
        lcd.print(idx);
    }

    if (buttonMapping.count(idx) > 0) {
        Keyboard.release(buttonMapping[idx]);
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
        lockLeds |= 16;
    }
    if (BootKeyboard.getLeds() & LED_NUM_LOCK) {
        lockLeds |= 128;
    }
    if (BootKeyboard.getLeds() & LED_SCROLL_LOCK) {
        lockLeds |= 1;
    }
    if (ledTimeout == 0) {
        setLeds(lockLeds, 0);
    }
}
