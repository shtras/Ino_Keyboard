#pragma once
#include "LCD.h"

extern "C" {
    void printMatrix();
    void readMatrix();
    void setupKeyboard();
    void checkLocks();
}

