#pragma once

struct Settings;
extern "C" Settings settings;

struct Settings
{
    bool keyPressEcho = false;
    bool irDebug = false;
    bool ledsEnabled = true;
    bool lcdEnabled = true;
    bool irEnabled = true;
};
