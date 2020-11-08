#include <Arduino.h>
#include <FastLED.h>
#include "ks.hpp"
#include "Effects.hpp"

Effect* effects[] = {
    new Off(),
    new BLEConnected(),
    new BLEDisconnected(),
    new BLEDebugOn(),
    new BLEDebugOff(),
    new BLEError(),
    new BLESearching(),
    new LeftOff(),
    new RightOff(),
    new HalfHalf(),
    new Segment(),
    new DC17(),
    new Director(),
    new Blinker(),
    new Brake(),
    new Backup(),
    new RainbowWave(),
    new Music(),
    new Trail(),
    new Solid()
};

typedef enum {
    E_OFF               = 0,
    E_BLE_CONNECTED     = 1,
    E_BLE_DISCONNECTED  = 2,
    E_BLE_DEBUG_ON      = 3,
    E_BLE_DEBUG_OFF     = 4,
    E_BLE_ERROR         = 5,
    E_BLE_SEARCHING     = 6,
    E_LEFT_OFF          = 7,
    E_RIGHT_OFF         = 8,
    E_HALFHALF          = 9,
    E_SEGMENT           = 10,
    E_DC17              = 11,
    E_DIRECT            = 12,
    E_BLINKER           = 13,
    E_BRAKE             = 14,
    E_BACKUP            = 15,
    E_RAINBOW_WAVE      = 16,
    E_MUSIC             = 17,
    E_TRAIL             = 18,
    E_SOLID             = 19
} EffectIndex;
EffectIndex activeEffect, oldEffect;

bool driving = true;
bool police = false;
bool brake = false;
// Does not need to be set to 0; data is cleared in setup()

// LED strips
CRGB back[NUM_LEDS];
CRGB left[NUM_RB_LEDS];
CRGB right[NUM_RB_LEDS];
CRGB *leds[] = { back, left, right };

// Other functions defined after loop() and setup()
void switchEffect(EffectIndex);
void playQuickEffect(EffectIndex);
void clearNoShow();
void ClearSerial();
void checkPi();
bool updateStrobe();
void updateDriving(long);

void setup() {

    Serial.begin(BAUD_RATE);
    Serial.setTimeout(SERIAL_TIMEOUT);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    pinMode(DATA_REQUEST_PIN, INPUT);
    pinMode(DATA_REQUEST_READY_PIN, OUTPUT);
    digitalWrite(DATA_REQUEST_READY_PIN, LOW);
    pinMode(CONTROL_CHECK_PIN, INPUT);

    FastLED.addLeds<WS2812, BACK_LED_PIN, GRB>(back, NUM_LEDS);
    FastLED.addLeds<WS2811, LEFT_LED_PIN, BRG>(left, NUM_RB_LEDS);
    FastLED.addLeds<WS2811, RIGHT_LED_PIN, BRG>(right, NUM_RB_LEDS);
    FastLED.clear();
    FastLED.show();
    switchEffect(E_OFF);
    Startup *startupEffect = new Startup();
    while (startupEffect->step(leds)) { FastLED.show(); }
    FastLED.show();
    FastLED.setBrightness(255);
    FastLED.clear();
}

void loop() {
    
    checkPi();
    long current = millis();
    if (driving) updateDriving(current);
    if (effects[activeEffect]->step(leds, current, updateStrobe(), police, brake)) switchEffect(oldEffect);
    if (driving) {

        if (digitalRead(A4)) for (int i = 1; i < 3; i++) for (int j = 0; j < NUM_RB_LEDS; j++) leds[i][j] = AMBER;
        else for (int i = 1; i < 3; i++) for (int j = 0; j < NUM_RB_LEDS; j++) leds[i][j] = OFF;
    }
    FastLED.show();
}

void switchEffect(EffectIndex modeToSwitchTo) {

    if (activeEffect != modeToSwitchTo) {

        activeEffect = modeToSwitchTo;
        effects[activeEffect]->reset();
    }
}

void playQuickEffect(EffectIndex effectToPlay) {
    
    if (effectToPlay != activeEffect) {

        oldEffect = activeEffect;
        switchEffect(effectToPlay);
    }
}

void clearNoShow() {

    for (int i = 0; i < NUM_LEDS; i++)   back[i] = OFF;
    for (int i = 0; i < NUM_RB_LEDS; i++) left[i] = OFF;
    for (int i = 0; i < NUM_RB_LEDS; i++) right[i] = OFF;
}

bool updateStrobe() {

    static bool strobeToggle = false;
    static long timer = millis();

    long current = millis();
    if (current - timer >= STROBE_MS) {

        timer = current;
        strobeToggle = !strobeToggle;
    }
    return strobeToggle;
}

void checkPi() {
    if (!SKIP_TEATHER_CHECK && !digitalRead(CONTROL_CHECK_PIN)) driving = true;
    else if (digitalRead(DATA_REQUEST_PIN)) {
        ClearSerial();
        // Tell control its okay to send data now
        digitalWrite(DATA_REQUEST_READY_PIN, HIGH);
        char rec[4] = { 0 };
        int nrec = Serial.readBytes(rec, 4);
        ClearSerial();
        digitalWrite(DATA_REQUEST_READY_PIN, LOW);
        if (nrec == 4) {

            driving = rec[0];
            switchEffect((EffectIndex)((int)rec[1]));
            police = rec[2];
            if (rec[3] != 1) ((Director*)effects[E_DIRECT])->m_direction = rec[3] == 0 ? 2 : 0;
        }
        else driving = true;
    }
}

bool fakeBlink() {

    static long lastBlink = millis();
    static bool blink = false;
    long current = millis();
    if (current - lastBlink > BLINK_CYCLE_MS / 2) {

        blink = !blink;
        lastBlink = current;
    }
    return blink;
}

void updateDriving(long current) {

    static long timer;
    static bool waitingForBrake = false;

    if (waitingForBrake) {

        if (millis() - timer >= 75) {

            waitingForBrake = false;
        }
    }
    if (!waitingForBrake) {

        bool old_brake = brake;
        brake = digitalRead(A0);
        if (old_brake != brake) {
            
            waitingForBrake = true;
            timer = millis();
        }
        else {
        
            const bool left = digitalRead(A1);
            const bool right = digitalRead(A2);
            const bool backup = digitalRead(A3);
            Blinker* blinker = (Blinker*)effects[E_BLINKER];

            if (backup) switchEffect(E_BACKUP);
            else {

                if (brake) {

                    if (blinker->m_left_blinking) {

                        if (!left) {

                            blinker->m_on_off_cycle = true;
                            switchEffect(E_LEFT_OFF);
                            blinker->m_last_left = current;
                        }
                        else if (current - blinker->m_last_left >= BLINK_MAX_OFF_W_TOLERANCE) {

                            blinker->m_left_blinking = false;
                            switchEffect(E_OFF);
                        }
                        else {

                            blinker->m_was_on_off_cycle = blinker->m_on_off_cycle;
                            blinker->m_on_off_cycle = false;
                            if (blinker->m_was_on_off_cycle) {

                                switchEffect(E_BLINKER);
                                blinker->m_direction = 0;
                            }
                        }
                    }
                    else {

                        if (!left && !blinker->m_right_blinking) {

                            blinker->m_right_blinking = false;
                            blinker->m_left_blinking = true;
                            blinker->m_last_left = current;
                            switchEffect(E_BLINKER);
                            blinker->m_direction = 0;
                        }
                    }

                    if (blinker->m_right_blinking) {

                        if (!right) {

                            blinker->m_on_off_cycle = true;
                            switchEffect(E_RIGHT_OFF);
                            blinker->m_last_right = current;
                        }
                        else if (current - blinker->m_last_right >= BLINK_MAX_OFF_W_TOLERANCE) {

                            blinker->m_right_blinking = false;
                            switchEffect(E_OFF);
                        }
                        else {

                            blinker->m_was_on_off_cycle = blinker->m_on_off_cycle;
                            blinker->m_on_off_cycle = false;
                            if (blinker->m_was_on_off_cycle) {

                                switchEffect(E_BLINKER);
                                blinker->m_direction = 1;
                            }
                        }
                    }
                    else {

                        if (!right && !blinker->m_left_blinking) {

                            blinker->m_left_blinking = false;
                            blinker->m_right_blinking = true;
                            blinker->m_last_right = current;
                            switchEffect(E_BLINKER);
                            blinker->m_direction = 1;
                        }
                    }

                    if (!blinker->m_left_blinking && !blinker->m_right_blinking) switchEffect(E_BRAKE);
                }
                else {

                    if (blinker->m_left_blinking) {

                        if (left) {
                            
                            blinker->m_was_on_off_cycle = blinker->m_on_off_cycle;
                            blinker->m_on_off_cycle = false;
                            if (blinker->m_was_on_off_cycle) {

                                switchEffect(E_BLINKER);
                                blinker->m_direction = 0;
                            }
                            blinker->m_last_left = current;
                        }
                        else if (current - blinker->m_last_left >= BLINK_MAX_OFF_W_TOLERANCE) {

                            blinker->m_left_blinking = false;
                            switchEffect(E_OFF);
                        }
                        else {

                            //if we are still considered blinking, and we do not detect
                            //the left light, then we are in the off cycle
                            blinker->m_on_off_cycle = true;
                            switchEffect(E_LEFT_OFF);
                        }
                    }
                    else {

                        if (left && !blinker->m_right_blinking) {

                            blinker->m_right_blinking = false;
                            blinker->m_left_blinking = true;
                            blinker->m_last_left = current;
                            switchEffect(E_BLINKER);
                            blinker->m_direction = 0;
                        }
                    }

                    if (blinker->m_right_blinking) {

                        if (right) {
                            
                            blinker->m_was_on_off_cycle = blinker->m_on_off_cycle;
                            blinker->m_on_off_cycle = false;
                            if (blinker->m_was_on_off_cycle) {

                                switchEffect(E_BLINKER);
                                blinker->m_direction = 1;
                            }
                            blinker->m_last_right = current;
                        }
                        else if (current - blinker->m_last_right >= BLINK_MAX_OFF_W_TOLERANCE) {

                            blinker->m_right_blinking = false;
                            switchEffect(E_OFF);
                        }
                        else {

                            //if we are still considered blinking, and we do not detect
                            //the right light, then we are in the off cycle
                            blinker->m_on_off_cycle = true;
                            switchEffect(E_RIGHT_OFF);
                        }
                    }
                    else {

                        if (right && !blinker->m_left_blinking) {

                            blinker->m_left_blinking = false;
                            blinker->m_right_blinking = true;
                            blinker->m_last_right = current;
                            switchEffect(E_BLINKER);
                            blinker->m_direction = 1;
                        }
                    }

                    if (!blinker->m_left_blinking && !blinker->m_right_blinking) switchEffect(E_OFF);
                }
            }
        }
    }
}