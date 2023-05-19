#include "LCD.h"
#include "Leds.h"
#include "Out.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "Arduino.h"
// When the display powers up, it is configured as follows:
//
// 1. Display clear
// 2. Function set:
//    DL = 1; 8-bit interface data
//    N = 0; 1-line display
//    F = 0; 5x8 dot character font
// 3. Display on/off control:
//    D = 0; Display off
//    C = 0; Cursor off
//    B = 0; Blinking off
// 4. Entry mode set:
//    I/D = 1; Increment by 1
//    S = 0; No shift
//
// Note, however, that resetting the Arduino doesn't reset the LCD, so we
// can't assume that its in that state when a sketch starts (and the
// LiquidCrystal constructor is called).

LiquidCrystal::LiquidCrystal(uint8_t latchPin, uint8_t clockPin, uint8_t dataPin)
    : latchPin_(latchPin)
    , clockPin_(clockPin)
    , dataPin_(dataPin)
{
    init();
}

void LiquidCrystal::init()
{
    displayFunction_ = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;

    begin(16, 2);
}

void LiquidCrystal::begin(uint8_t cols, uint8_t lines, uint8_t dotsize)
{
    if (lines > 1) {
        displayFunction_ |= LCD_2LINE;
    }
    numLines_ = lines;

    setRowOffsets(0x00, 0x40, 0x00 + cols, 0x40 + cols);

    // for some 1 line displays you can select a 10 pixel high font
    if ((dotsize != LCD_5x8DOTS) && (lines == 1)) {
        displayFunction_ |= LCD_5x10DOTS;
    }

    // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
    // according to datasheet, we need at least 40ms after power rises above 2.7V
    // before sending commands. Arduino can turn on way before 4.5V so we'll wait 50
    delayMicroseconds(50000);
    // Now we pull both RS and R/W low to begin commands
    rsPin_ = LOW;
    enablePin_ = LOW;

    // this is according to the hitachi HD44780 datasheet
    // figure 24, pg 46

    // we start in 8bit mode, try to set 4 bit mode
    write4bits(0x03);
    delayMicroseconds(4500); // wait min 4.1ms

    // second try
    write4bits(0x03);
    delayMicroseconds(4500); // wait min 4.1ms

    // third go!
    write4bits(0x03);
    delayMicroseconds(150);

    // finally, set to 4-bit interface
    write4bits(0x02);

    // finally, set # lines, font size, etc.
    command(LCD_FUNCTIONSET | displayFunction_);

    // turn the display on with no cursor or blinking default
    displayControl_ = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    display();

    // clear it off
    clear();

    // Initialize to default text direction (for romance languages)
    displayMode_ = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    // set the entry mode
    command(LCD_ENTRYMODESET | displayMode_);
}

void LiquidCrystal::setRowOffsets(int row0, int row1, int row2, int row3)
{
    rowOffsets_[0] = row0;
    rowOffsets_[1] = row1;
    rowOffsets_[2] = row2;
    rowOffsets_[3] = row3;
}

/********** high level commands, for the user! */
void LiquidCrystal::clear()
{
    command(LCD_CLEARDISPLAY); // clear display, set cursor position to zero
    delayMicroseconds(2000);   // this command takes a long time!
}

void LiquidCrystal::home()
{
    command(LCD_RETURNHOME); // set cursor position to zero
    delayMicroseconds(2000); // this command takes a long time!
}

void LiquidCrystal::setCursor(uint8_t col, uint8_t row)
{
    const size_t max_lines = sizeof(rowOffsets_) / sizeof(*rowOffsets_);
    if (row >= max_lines) {
        row = max_lines - 1; // we count rows starting w/0
    }
    if (row >= numLines_) {
        row = numLines_ - 1; // we count rows starting w/0
    }

    command(LCD_SETDDRAMADDR | (col + rowOffsets_[row]));
}

// Turn the display on/off (quickly)
void LiquidCrystal::noDisplay()
{
    displayControl_ &= ~LCD_DISPLAYON;
    command(LCD_DISPLAYCONTROL | displayControl_);
}
void LiquidCrystal::display()
{
    displayControl_ |= LCD_DISPLAYON;
    command(LCD_DISPLAYCONTROL | displayControl_);
}

// Turns the underline cursor on/off
void LiquidCrystal::noCursor()
{
    displayControl_ &= ~LCD_CURSORON;
    command(LCD_DISPLAYCONTROL | displayControl_);
}
void LiquidCrystal::cursor()
{
    displayControl_ |= LCD_CURSORON;
    command(LCD_DISPLAYCONTROL | displayControl_);
}

// Turn on and off the blinking cursor
void LiquidCrystal::noBlink()
{
    displayControl_ &= ~LCD_BLINKON;
    command(LCD_DISPLAYCONTROL | displayControl_);
}
void LiquidCrystal::blink()
{
    displayControl_ |= LCD_BLINKON;
    command(LCD_DISPLAYCONTROL | displayControl_);
}

// These commands scroll the display without changing the RAM
void LiquidCrystal::scrollDisplayLeft(void)
{
    command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void LiquidCrystal::scrollDisplayRight(void)
{
    command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void LiquidCrystal::leftToRight(void)
{
    displayMode_ |= LCD_ENTRYLEFT;
    command(LCD_ENTRYMODESET | displayMode_);
}

// This is for text that flows Right to Left
void LiquidCrystal::rightToLeft(void)
{
    displayMode_ &= ~LCD_ENTRYLEFT;
    command(LCD_ENTRYMODESET | displayMode_);
}

// This will 'right justify' text from the cursor
void LiquidCrystal::autoscroll(void)
{
    displayMode_ |= LCD_ENTRYSHIFTINCREMENT;
    command(LCD_ENTRYMODESET | displayMode_);
}

// This will 'left justify' text from the cursor
void LiquidCrystal::noAutoscroll(void)
{
    displayMode_ &= ~LCD_ENTRYSHIFTINCREMENT;
    command(LCD_ENTRYMODESET | displayMode_);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void LiquidCrystal::createChar(uint8_t location, uint8_t charmap[])
{
    location &= 0x7; // we only have 8 locations 0-7
    command(LCD_SETCGRAMADDR | (location << 3));
    for (int i = 0; i < 8; i++) {
        write(charmap[i]);
    }
}

/*********** mid level commands, for sending data/cmds */

inline void LiquidCrystal::command(uint8_t value)
{
    send(value, LOW);
}

inline size_t LiquidCrystal::write(uint8_t value)
{
    send(value, HIGH);
    return 1; // assume sucess
}

/************ low level data pushing commands **********/

void LiquidCrystal::flushPins()
{
    int value = (rsPin_ << 7) | (enablePin_ << 5) | (dataPins_[0] << 4) | (dataPins_[1] << 3) |
                (dataPins_[2] << 2) | (dataPins_[3] << 1);
    pinMode(latchPin_, OUTPUT);
    pinMode(dataPin_, OUTPUT);
    pinMode(clockPin_, OUTPUT);
    digitalWrite(latchPin_, LOW);
    shiftOut(dataPin_, clockPin_, LSBFIRST, value);
    digitalWrite(latchPin_, HIGH);
}

// write either command or data, with automatic 4/8-bit selection
void LiquidCrystal::send(uint8_t value, uint8_t mode)
{
    rsPin_ = mode;
    flushPins();

    write4bits(value >> 4);
    write4bits(value);
}

void LiquidCrystal::pulseEnable(void)
{
    enablePin_ = LOW;
    flushPins();
    delayMicroseconds(1);

    enablePin_ = HIGH;
    flushPins();
    delayMicroseconds(1); // enable pulse must be >450ns

    enablePin_ = LOW;
    flushPins();
    delayMicroseconds(100); // commands need > 37us to settle
}

void LiquidCrystal::write4bits(uint8_t value)
{
    for (int i = 0; i < 4; i++) {
        dataPins_[i] = (value >> i) & 0x01;
    }

    pulseEnable();
}

namespace
{
const int bounceInterval = 450;
}

void LiquidCrystal::displayLoop()
{
    if (displayTimeout_ == 0) {
        upper_.bounce();
        lower_.bounce();
    }
    if (displayTimeout_ > 0 && displayTimeout_ <= millis()) {
        displayTimeout_ = 0;
        upper_.reset();
        lower_.reset();
    }
    out::cout << "Display timeout = " << displayTimeout_ << out::endl;
}

void LiquidCrystal::setPersistentStrings(
    const __FlashStringHelper* upper, const __FlashStringHelper* lower)
{
    clear();
    displayTimeout_ = millis();
    upper_.set(upper);
    lower_.set(lower);
}

void LiquidCrystal::setTimeout(unsigned long value)
{
    displayTimeout_ = value;
}

void displayLoop()
{
    lcd.displayLoop();
}

LiquidCrystal::BouncyStr::BouncyStr(int row)
    : row_(row), bounceTime_(millis() + bounceInterval)
{
}

void LiquidCrystal::BouncyStr::bounce()
{
    if (len_ <= 16) {
        return;
    }
    if (bounceTime_ > millis()) {
        return;
    }
    bounceTime_ = millis() + bounceInterval;

    if (dir_ < 0 && idx_ <= 0) {
        dir_ = 1;
    }
    if (dir_ > 0 && idx_ >= len_ - 16) {
        dir_ = -1;
    }
    idx_ += dir_;
    display();
}

void LiquidCrystal::BouncyStr::set(const __FlashStringHelper* val)
{
    len_ = strlen_P(reinterpret_cast<const char*>(val));
    strncpy_P(str_, reinterpret_cast<const char*>(val), sizeof(str_) - 1);
    display();
}

void LiquidCrystal::BouncyStr::reset()
{
    idx_ = 0;
    display();
}

void LiquidCrystal::BouncyStr::display()
{
    lcd.setCursor(0, row_);
    auto c = str_[idx_ + 16];
    str_[idx_ + 16] = '\0';
    lcd.print(str_ + idx_, 0);
    str_[idx_ + 16] = c;
}
