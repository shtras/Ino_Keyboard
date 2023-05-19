#pragma once
#ifndef LCD_h
#define LCD_h

#include <inttypes.h>
#include "Print.h"

class LiquidCrystal;
extern "C" LiquidCrystal lcd;

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

extern "C" {
void displayLoop();
}

class LiquidCrystal : public Print
{
public:
    LiquidCrystal(uint8_t latchPin, uint8_t clockPin, uint8_t dataPin);

    void init();

    void begin(uint8_t cols, uint8_t rows, uint8_t charsize = LCD_5x8DOTS);

    void clear();
    void home();

    void noDisplay();
    void display();
    void noBlink();
    void blink();
    void noCursor();
    void cursor();
    void scrollDisplayLeft();
    void scrollDisplayRight();
    void leftToRight();
    void rightToLeft();
    void autoscroll();
    void noAutoscroll();

    void setRowOffsets(int row1, int row2, int row3, int row4);
    void createChar(uint8_t, uint8_t[]);
    void setCursor(uint8_t, uint8_t);
    virtual size_t write(uint8_t);
    void command(uint8_t);

    template <typename T>
    void print(const T val, int timeout = 1000)
    {
        Print::print(val);
        if (timeout > 0) {
            _displayTimeout = millis() + timeout;
        }
    }
    void setPersistentStrings(const __FlashStringHelper* upper, const __FlashStringHelper* lower);
    void setTimeout(unsigned long value);
    void displayLoop();

    using Print::write;

private:
    void flushPins();

    void send(uint8_t, uint8_t);
    void write4bits(uint8_t);
    void pulseEnable();

    uint8_t _rs_pin;     // LOW: command.  HIGH: character.
    uint8_t _enable_pin; // activated by a HIGH pulse.
    uint8_t _data_pins[4];

    uint8_t _displayfunction;
    uint8_t _displaycontrol;
    uint8_t _displaymode;

    uint8_t _initialized;

    uint8_t _numlines;
    uint8_t _row_offsets[4];

    uint8_t latchPin_;
    uint8_t clockPin_;
    uint8_t dataPin_;

    char _persistentBufferUp[16] = {0};
    char _persistentBufferDn[16] = {0};
    unsigned long _displayTimeout = 0;
};

#endif
