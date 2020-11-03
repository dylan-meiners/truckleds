#include <Arduino.h>
#include "Utils.hpp"

int RoundLit(double x) {
    
    if (x >= 0) { return (x - (int)x >= .5) ? (int)x + 1 : (int)x; }
    else { return -((-x - (int)-x >= .5) ? (int)-x + 1 : (int)-x); }
}

void ClearSerial() {

    while (Serial.available() > 0) { char ch = Serial.read(); }
}

int getLength(int index) {

    if (index == 0) return NUM_LEDS;
    else return NUM_RB_LEDS;
}