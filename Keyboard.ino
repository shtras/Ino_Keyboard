#include "HID-Project.h"
#include "IRremote.h"

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

std::map<int, ConsumerKeycode> irMapping = {
    {0x45, MEDIA_VOL_MUTE}, {0x44, MEDIA_PREVIOUS}, {0x40, MEDIA_PLAY_PAUSE}, {0x43, MEDIA_NEXT}};

std::map<int, ConsumerKeycode> consumerMapping = {{13, CONSUMER_CALCULATOR}};

namespace pins
{
const int potentiometer = A0;
const int ledsClock = 7;
const int ledsLatch = 8;
const int ledsData = 9;
const int encoderA = 1;
const int encoderB = 0;
const int encoderBtn = 10;
const int ir = 16;
} // namespace pins

void encoderISR();

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
    pinMode(pins::ledsClock, OUTPUT);
    pinMode(pins::ledsData, OUTPUT);
    pinMode(pins::ledsLatch, OUTPUT);
    pinMode(pins::encoderBtn, INPUT_PULLUP);
    Keyboard.begin();
    Consumer.begin();
    BootKeyboard.begin();
    pinMode(pins::encoderA, INPUT);
    pinMode(pins::encoderB, INPUT);
    digitalWrite(pins::encoderA, HIGH);
    digitalWrite(pins::encoderB, HIGH);
    attachInterrupt(digitalPinToInterrupt(pins::encoderA), encoderISR, CHANGE);
    IrReceiver.begin(pins::ir, ENABLE_LED_FEEDBACK);
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
    processButton(4, digitalRead(pins::encoderBtn));
    return;
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

double getTargetVolume()
{
    int val = abs(1024 - analogRead(pins::potentiometer));
    double targetVolume = val / 1024.0;
    return targetVolume;
}

unsigned long ledTimeout = 0;
int persistentLedValue = 0;
void setLeds(int value, int timeout)
{
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
    //double targetVolume = getTargetVolume();
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
    }
}

volatile int lastEncoderA = 0;
void encoderISR()
{
    noInterrupts();
    using namespace volume;
    int A = digitalRead(pins::encoderA); //MSB = most significant bit
    int B = digitalRead(pins::encoderB); //LSB = least significant bit
    if (A != 0 && A != lastEncoderA) {
        if (A == B) {
            //changeVolume(MEDIA_VOLUME_UP);
            targetVolume += 0.02;
            out::cout << "CW" << out::endl;
        } else {
            //changeVolume(MEDIA_VOLUME_DOWN);
            targetVolume -= 0.02;
            out::cout << "CCW" << out::endl;
        }
    }
    lastEncoderA = A;
    interrupts();
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
    if (ledTimeout && ledTimeout < millis()) {
        setLeds(persistentLedValue, 0);
    }
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
    out::cout << out::hex << data.command << " " << lastCommand << " " << out::dec << commandRepeat
              << " " << repeat << out::endl;

    if (data.command != lastCommand) {
        commandRepeat = 0;
        return;
    }
    if (!repeat) {
        commandRepeat = 0;
    }
    ++commandRepeat;
    if (commandRepeat >= continuousThreshold) {
        out ::cout << "Continuous" << out::endl;
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
        out::cout << "Single" << out::endl;
        if (irMapping.count(data.command) > 0) {
            setLeds(255, 100);
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
    {&blinkLed, 1000, 0}, {&printMatrix, 100, 0}, {&checkLocks, 100, 0}, {&checkIR, 100, 0}};

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
