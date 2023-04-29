#include <HID-Project.h>
#include <IRremote.h>
#include <ArduinoSTL.h>

#include "LiquidCrystal.h"
#include "Out.h"
#include "Matrix.h"

#include <array>
#include <map>

constexpr int numRows = 5;
constexpr int numCols = 4;
std::array<int, numRows> rows = {10, 6, 5, 15, 14};
std::array<int, numCols> cols = {21, 20, 19, 18};
std::array<byte, numRows * numCols> buttonState;

std::map<int, KeyboardKeycode> buttonMapping = {{17, KEY_0}, {3, KEYPAD_DOT}, {16, KEYPAD_1},
    {18, KEYPAD_2}, {19, KEYPAD_3}, {15, KEYPAD_4}, {11, KEYPAD_5}, {7, KEYPAD_6}, {14, KEYPAD_7},
    {10, KEYPAD_8}, {6, KEYPAD_9}, {2, KEYPAD_ENTER}, {1, KEYPAD_ADD}, {0, KEYPAD_SUBTRACT},
    {9, KEYPAD_DIVIDE}, {5, KEYPAD_MULTIPLY}};

std::map<int, ConsumerKeycode> irMapping = {
    {0x45, MEDIA_VOL_MUTE}, {0x44, MEDIA_PREVIOUS}, {0x40, MEDIA_PLAY_PAUSE}, {0x43, MEDIA_NEXT}};

std::map<int, ConsumerKeycode> consumerMapping = {{13, CONSUMER_CALCULATOR}};

namespace pins
{
const int ledsClock = 7;
const int ledsLatch = 8;
const int ledsData = 9;
const int encoderA = 1;
const int encoderB = 0;
const int ir = 16;

const int displayData = 2;
const int displayLatch = 3;
const int displayClock = 4;
} // namespace pins

LiquidCrystal lcd(pins::displayLatch, pins::displayClock, pins::displayData);

void encoderISR();

void setup()
{
    if (out::Enabled) {
        out::cout.Init();
    }

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
    pinMode(pins::ledsClock, OUTPUT);
    pinMode(pins::ledsData, OUTPUT);
    pinMode(pins::ledsLatch, OUTPUT);
    Keyboard.begin();
    Consumer.begin();
    BootKeyboard.begin();
    pinMode(pins::encoderA, INPUT);
    pinMode(pins::encoderB, INPUT);
    digitalWrite(pins::encoderA, HIGH);
    digitalWrite(pins::encoderB, HIGH);
    attachInterrupt(digitalPinToInterrupt(pins::encoderA), encoderISR, CHANGE);
    IrReceiver.begin(pins::ir, ENABLE_LED_FEEDBACK);
    lcd.begin(16, 2);
    lcd.print(F("Hello, World!"));
}

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

std::array<int, 8> ledAnimation = {0};
int ledAnimationCounter = 0;
int ledAnimationLength = 0;
int ledAnimationTimeout = 0;

void setLedAnimation(std::array<int, 8> animation, int length, int timeout)
{
    for (int i = 0; i < length; ++i) {
        ledAnimation[i] = animation[i];
    }
    ledAnimationLength = length;
    ledAnimationCounter = 0;
    ledAnimationTimeout = timeout;
}

unsigned long ledTimeout = 0;
int persistentLedValue = 0;
void setLeds(int value, int timeout)
{
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
    if (ledTimeout && ledTimeout < millis()) {
        setLeds(persistentLedValue, 0);
    }
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

namespace volume
{
double volumeValue = 0.0;
volatile double targetVolume = 0.0;
double minVolume = 0.0;
int ups = 0;
int downs = 0;
} // namespace volume

void changeVolume(ConsumerKeycode val)
{
    using namespace volume;
    Consumer.press(val);
    delay(10);
    Consumer.release(val);
    delay(10);
};

void checkVolume()
{
    using namespace volume;
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
            ledsValue |= 1 << (7 - i);
        }
        setLeds(ledsValue, 3000);
        lcd.setCursor(0, 1);
        lcd.print("Vol:            ");
        lcd.setCursor(5, 1);
        lcd.print(static_cast<int>((volumeValue - minVolume) * 100.0));
    }
}

volatile int lastEncoderA = 0;
void encoderISR()
{
    noInterrupts();
    using namespace volume;
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

decode_results results;
uint16_t lastCommand = 0;
uint16_t commandRepeat = 0;

struct IrGuard
{
public:
    explicit IrGuard(uint16_t command)
        : command_(command)
    {
    }
    ~IrGuard()
    {
        IrReceiver.resume();
        lastCommand = command_;
    }

private:
    uint16_t command_ = 0;
};

void checkIR()
{
    constexpr int continuousThreshold = 1;
    constexpr int singleThreshold = 2;
    if (!IrReceiver.decode()) {
        return;
    }
    const auto& data = IrReceiver.decodedIRData;
    IrGuard guard(data.command);
    bool repeat = data.flags & IRDATA_FLAGS_IS_REPEAT;
    out::cout << out::hex << data.command << F(" ") << lastCommand << F(" ") << out::dec << commandRepeat
              << F(" ") << repeat << out::endl;

    if (data.command != lastCommand) {
        commandRepeat = 0;
        return;
    }
    if (!repeat) {
        commandRepeat = 0;
    }
    lcd.setCursor(0, 0);
    lcd.print("C:    R:        ");
    lcd.setCursor(3, 0);
    lcd.print(data.command, HEX);
    lcd.setCursor(9, 0);
    lcd.print(repeat);
    ++commandRepeat;
    if (commandRepeat >= continuousThreshold) {
        out ::cout << F("Continuous") << out::endl;
        switch (data.command) {
            case 0x46:
                volume::targetVolume += 0.02;
                break;
            case 0x15:
                volume::targetVolume -= 0.02;
                break;
        }
    }
    if (commandRepeat == singleThreshold) {
        out::cout << F("Single") << out::endl;
        if (irMapping.count(data.command) > 0) {
            setLedAnimation({129, 66, 36, 24, 36, 66, 129, 0}, 7, 200);
            Consumer.write(irMapping[data.command]);
        }
    }
}

struct ScheduledFunction
{
    typedef void (*FunPtr)();
    FunPtr func;
    int interval;
    unsigned long last;
};

ScheduledFunction scheduledFuncs[] = {{&readMatrix, 1, 0}, {&checkVolume, 100, 0},
    {&blinkLed, 150, 0}, {&printMatrix, 400, 0}, {&checkLocks, 100, 0}, {&checkIR, 100, 0}};

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
