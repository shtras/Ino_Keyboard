#include "IR.h"
#include "Out.h"
#include "Volume.h"
#include "LCD.h"
#include "Leds.h"

#include <IRremote.h>
#include <ArduinoSTL.h>
#include <MultiReport/Consumer.h>
#include <map>

namespace
{
namespace pins
{
const int ir = 16;
}

std::map<int, ConsumerKeycode> irMapping = {
    {0x45, MEDIA_VOL_MUTE}, {0x44, MEDIA_PREVIOUS}, {0x40, MEDIA_PLAY_PAUSE}, {0x43, MEDIA_NEXT}};

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
} // namespace

void setupIR()
{
    IrReceiver.begin(pins::ir, ENABLE_LED_FEEDBACK);
}

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
    out::cout << out::hex << data.command << F(" ") << lastCommand << F(" ") << out::dec
              << commandRepeat << F(" ") << repeat << out::endl;

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
                addTargetVolume(0.02);
                break;
            case 0x15:
                addTargetVolume(-0.02);
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