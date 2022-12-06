#include "HID-Project.h"
#include "Out.h"
// Keyboard Matrix Tutorial Example
// baldengineer.com
// CC BY-SA 4.0

// JP1 is an input
byte rows[] = {2, 3, 4};
const int rowCount = sizeof(rows) / sizeof(rows[0]);

// JP2 and JP3 are outputs
byte cols[] = {8, 9, 10};
const int colCount = sizeof(cols) / sizeof(cols[0]);

namespace pins
{
const int potentiometer = A0;
const int joyX = A2;
const int joyY = A1;
const int joyBtn = 15;
} // namespace pins

byte keys[colCount][rowCount];

void setup()
{
    out::cout.Init();

    for (int x = 0; x < rowCount; x++) {
        out::cout << rows[x] << " as input";
        pinMode(rows[x], INPUT);
    }

    for (int x = 0; x < colCount; x++) {
        out::cout << cols[x] << " as input-pullup";
        pinMode(cols[x], INPUT_PULLUP);
    }
    pinMode(pins::potentiometer, INPUT);
    pinMode(pins::joyBtn, INPUT_PULLUP);
    pinMode(pins::joyX, INPUT);
    pinMode(pins::joyY, INPUT);
    Mouse.begin();
}

void readMatrix()
{
    // iterate the columns
    for (int colIndex = 0; colIndex < colCount; colIndex++) {
        // col: set to output to low
        byte curCol = cols[colIndex];
        pinMode(curCol, OUTPUT);
        digitalWrite(curCol, LOW);

        // row: interate through the rows
        for (int rowIndex = 0; rowIndex < rowCount; rowIndex++) {
            byte rowCol = rows[rowIndex];
            pinMode(rowCol, INPUT_PULLUP);
            keys[colIndex][rowIndex] = digitalRead(rowCol);
            pinMode(rowCol, INPUT);
        }
        // disable the column
        pinMode(curCol, INPUT);
    }
}

void printMatrix()
{
    for (int rowIndex = 0; rowIndex < rowCount; rowIndex++) {
        if (rowIndex < 10)
            out::cout << F("0");
        out::cout << rowIndex << F(": ");

        for (int colIndex = 0; colIndex < colCount; colIndex++) {
            out::cout << keys[colIndex][rowIndex];
            if (colIndex < colCount)
                out::cout << F(", ");
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
    {&checkMouse, 10, 0}, {&checkVolume, 100, 0}, {&blinkLed, 1000, 0}};

void loop()
{
    auto currTime = millis();
    for (int i = 0; i < sizeof(scheduledFuncs) / sizeof(ScheduledFunction); ++i) {
        if (currTime - scheduledFuncs[i].last > scheduledFuncs[i].interval) {
            scheduledFuncs[i].func();
            scheduledFuncs[i].last = currTime;
        }
    }
    //delay(1000);
}
