#pragma once
#ifndef LCD_h
#define LCD_h

#include <inttypes.h>
#include "Print.h"
#include "Settings.h"

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
enum class BounceType{Bounce, Loop, None};
    LiquidCrystal(uint8_t latchPin, uint8_t clockPin, uint8_t dataPin);

    void init();

    void begin();

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
        if (!settings.lcdEnabled) {
            return;
        }
        Print::print(val);
        if (timeout > 0) {
            displayTimeout_ = millis() + timeout;
        }
    }

    template <typename T>
    void cprint(const T val)
    {
        if (!settings.lcdEnabled) {
            return;
        }
        clear();
        setCursor(0, 0);
        print(val);
    }
    void setPersistentStrings(const __FlashStringHelper* upper, const __FlashStringHelper* lower);
    void setTimeout(unsigned long value);
    void displayLoop();
    void shiftBounceType();
    BounceType getBounceType();

    using Print::write;

private:
    class BouncyStr
    {
    public:
    
        BouncyStr(int row);
        void bounce();
        void set(const __FlashStringHelper* val);
        void reset();

    private:
        void display();

        int row_ = 0;
        char str_[64];
        int idx_ = 0;
        int dir_ = 1;
        int len_ = 0;
        unsigned long bounceTime_ = 0;
    };

    void flushPins();

    void send(uint8_t, uint8_t);
    void write4bits(uint8_t);
    void pulseEnable();

    uint8_t rsPin_;     // LOW: command.  HIGH: character.
    uint8_t enablePin_; // activated by a HIGH pulse.
    uint8_t dataPins_[4];

    uint8_t displayControl_;
    uint8_t displayMode_;

    uint8_t rowOffsets_[4];

    uint8_t latchPin_;
    uint8_t clockPin_;
    uint8_t dataPin_;

    unsigned long displayTimeout_ = 0;
    BouncyStr upper_{0};
    BouncyStr lower_{1};
    BounceType bounceType_ = BounceType::Loop;
};

#endif
