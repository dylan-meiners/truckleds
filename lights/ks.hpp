#pragma once

#include <FastLED.h>

typedef unsigned int uint;

// Config
const bool SKIP_TEATHER_CHECK = true;

// General consts
const int NUM_STRIPS            = 3;
const int NUM_LEDS              = 96;
const int NUM_RB_LEDS           = 45;
const int NUM_LEDS_TOTAL        = NUM_LEDS + NUM_RB_LEDS * 2;
const int BACK_LED_PIN          = 7;
const int LEFT_LED_PIN          = 8;
const int RIGHT_LED_PIN         = 9;
const int CONTROL_CHECK_PIN     = 3;
const int DATA_REQUEST_PIN      = 2;
const int DATA_REQUEST_READY_PIN = 4;
const long BAUD_RATE            = 115200;
const int SERIAL_TIMEOUT        = 1000;

// Colors
const CRGB RED      = CRGB(255, 0, 0);
const CRGB BLUE     = CRGB(0, 0, 255);
const CRGB AMBER    = CRGB(255, 50, 0);
const CRGB WHITE    = CRGB(255, 255, 255);
const CHSV OFF      = CHSV(0, 0, 0);

// Effect parameters
const int STROBE_MS                 = 35;
const int BLINK_CYCLE_MS            = 750;
const int BLINK_MAX_OFF             = BLINK_CYCLE_MS / 2;
const int BLINK_MAX_OFF_W_TOLERANCE = BLINK_MAX_OFF * 1.1;
const double WAVE_SPEED_SCALAR      = 1.0;

// LEDs
typedef enum {
    BACK_LEDS   = 0,
    LEFT_LEDS   = 1,
    RIGHT_LEDS  = 2
} StripIndex;