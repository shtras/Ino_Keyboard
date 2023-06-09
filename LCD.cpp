#include "LCD.h"

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
    _displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;

    begin(16, 2);
}

void LiquidCrystal::begin(uint8_t cols, uint8_t lines, uint8_t dotsize)
{
    if (lines > 1) {
        _displayfunction |= LCD_2LINE;
    }
    _numlines = lines;

    setRowOffsets(0x00, 0x40, 0x00 + cols, 0x40 + cols);

    // for some 1 line displays you can select a 10 pixel high font
    if ((dotsize != LCD_5x8DOTS) && (lines == 1)) {
        _displayfunction |= LCD_5x10DOTS;
    }

    // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
    // according to datasheet, we need at least 40ms after power rises above 2.7V
    // before sending commands. Arduino can turn on way before 4.5V so we'll wait 50
    delayMicroseconds(50000);
    // Now we pull both RS and R/W low to begin commands
    _rs_pin = LOW;
    _enable_pin = LOW;

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
    command(LCD_FUNCTIONSET | _displayfunction);

    // turn the display on with no cursor or blinking default
    _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    display();

    // clear it off
    clear();

    // Initialize to default text direction (for romance languages)
    _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    // set the entry mode
    command(LCD_ENTRYMODESET | _displaymode);
}

void LiquidCrystal::setRowOffsets(int row0, int row1, int row2, int row3)
{
    _row_offsets[0] = row0;
    _row_offsets[1] = row1;
    _row_offsets[2] = row2;
    _row_offsets[3] = row3;
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
    const size_t max_lines = sizeof(_row_offsets) / sizeof(*_row_offsets);
    if (row >= max_lines) {
        row = max_lines - 1; // we count rows starting w/0
    }
    if (row >= _numlines) {
        row = _numlines - 1; // we count rows starting w/0
    }

    command(LCD_SETDDRAMADDR | (col + _row_offsets[row]));
}

// Turn the display on/off (quickly)
void LiquidCrystal::noDisplay()
{
    _displaycontrol &= ~LCD_DISPLAYON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystal::display()
{
    _displaycontrol |= LCD_DISPLAYON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void LiquidCrystal::noCursor()
{
    _displaycontrol &= ~LCD_CURSORON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystal::cursor()
{
    _displaycontrol |= LCD_CURSORON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void LiquidCrystal::noBlink()
{
    _displaycontrol &= ~LCD_BLINKON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystal::blink()
{
    _displaycontrol |= LCD_BLINKON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
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
    _displaymode |= LCD_ENTRYLEFT;
    command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void LiquidCrystal::rightToLeft(void)
{
    _displaymode &= ~LCD_ENTRYLEFT;
    command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void LiquidCrystal::autoscroll(void)
{
    _displaymode |= LCD_ENTRYSHIFTINCREMENT;
    command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void LiquidCrystal::noAutoscroll(void)
{
    _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
    command(LCD_ENTRYMODESET | _displaymode);
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
    int value = (_rs_pin << 7) | (_enable_pin << 5) | (_data_pins[0] << 4) |
                (_data_pins[1] << 3) | (_data_pins[2] << 2) | (_data_pins[3] << 1);
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
    _rs_pin = mode;
    flushPins();

    write4bits(value >> 4);
    write4bits(value);
}

void LiquidCrystal::pulseEnable(void)
{
    _enable_pin = LOW;
    flushPins();
    delayMicroseconds(1);

    _enable_pin = HIGH;
    flushPins();
    delayMicroseconds(1); // enable pulse must be >450ns

    _enable_pin = LOW;
    flushPins();
    delayMicroseconds(100); // commands need > 37us to settle
}

void LiquidCrystal::write4bits(uint8_t value)
{
    for (int i = 0; i < 4; i++) {
        _data_pins[i] = (value >> i) & 0x01;
    }

    pulseEnable();
}

void LiquidCrystal::displayLoop()
{
    if (_displayTimeout == 0) {
        return;
    }
    if (_displayTimeout > millis()) {
        return;
    }
    _displayTimeout = 0;
    clear();
    setCursor(0, 0);
    Print::print(_persistentBufferUp);
    setCursor(0, 1);
    Print::print(_persistentBufferDn);
}

void LiquidCrystal::setPersistentStrings(const __FlashStringHelper* upper, const __FlashStringHelper* lower)
{
    strncpy_P(_persistentBufferUp, reinterpret_cast<const char*>(upper), 16);
    strncpy_P(_persistentBufferDn, reinterpret_cast<const char*>(lower), 16);
    _displayTimeout = millis();
}

void LiquidCrystal::setTimeout(unsigned long value)
{
    _displayTimeout = value;
}

void displayLoop()
{
    lcd.displayLoop();
}
