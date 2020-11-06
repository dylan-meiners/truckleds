#pragma once

#include <Arduino.h>
#include <FastLED.h>
#include "ks.hpp"
#include "Utils.hpp"

class Effect {

    public:

        virtual bool step(CRGB **leds, long current, bool strobeToggle, bool police, bool brake) = 0;
        virtual void reset() = 0;
};

class Off : public Effect {
    
    public:

        bool step(CRGB **leds, long current, bool strobeToggle, bool police, bool brake) {

            FastLED.clear();
            return false;
        }
        void reset() {};
};

class Startup {

    public:

        // Not a public Effect. This one's step function returns false when
        // completed.
        bool step(CRGB **leds) {

            long current = millis();
            long delta = current - m_timer;

            if (!fading) {
                
                int totalLitRBNotRounded = (1.2448304835282 * pow(1.0085050662548, (double)(delta + 15)) - 1.4134533) / 3.0;
                int totalLitRB = RoundLit(totalLitRBNotRounded);
                if (totalLitRB < 0) totalLitRB = 0;
                else if (totalLitRB > 22) totalLitRB = 22;
                int toLight = totalLitRB - m_numLitRB;
                if (toLight != 0) {

                    for (int i = 1; i < 3; i++) {

                        for (int j = 0; j < toLight; j++) {

                            leds[i][NUM_RB_LEDS / 2 - 1 - m_numLitRB - j] = BLUE;
                            leds[i][NUM_RB_LEDS / 2 + m_numLitRB + j] = BLUE;
                        }
                        if (totalLitRB == 22) {

                            leds[i][NUM_RB_LEDS - 1] = BLUE;
                        }
                    }
                    m_numLitRB += toLight;
                }
                int totalLitBack = RoundLit(totalLitRBNotRounded / 22.0 * 48.0);
                if (totalLitBack < 0) totalLitBack = 0;
                else if (totalLitBack > 48) totalLitBack = 48;
                toLight = totalLitBack - m_numLitBack;
                if (toLight != 0) {

                    for (int i = 0; i < toLight; i++) {

                        leds[BACK_LEDS][NUM_LEDS / 2 - 1 - m_numLitBack - i] = BLUE;
                        leds[BACK_LEDS][NUM_LEDS / 2 + m_numLitBack + i] = BLUE;
                    }
                }

                FastLED.setBrightness(RoundLit(totalLitRB / 22.0 * 255.0));
                if (totalLitRB == 22) {

                    fading = true;
                    m_timer = current;
                }
            }
            else {

                int brightness = 255 - RoundLit(delta / 2000.0 * 255.0);
                FastLED.setBrightness(brightness);
                if (brightness == 0) return false;
            }
            return true;
        }

    private:

        long m_timer = millis();
        int m_numLitRB = 0;
        int m_numLitBack = 0;
        bool fading = false;
};

class BLEConnected : public Effect {

    public:

        bool step(CRGB **leds, long current, bool strobeToggle, bool police, bool brake) {

            if (m_next_ramp > 128) m_done = true;
            if (!m_done) {

                leds[BACK_LEDS][NUM_LEDS / 2 - 1 - m_next_ramp] = BLUE;
                leds[BACK_LEDS][NUM_LEDS / 2 + m_next_ramp] = BLUE;
                m_next_ramp++;
            }
            else return true;
            return false;
        }

        void reset() {

            m_done = false;
            m_next_ramp = 0;
        }

    private:

        bool m_done;
        int m_next_ramp;
};

class BLEDisconnected : public Effect {

    public:

        bool step(CRGB **leds, long current, bool strobeToggle, bool police, bool brake) {

            if (m_next_ramp < 0) m_done = true;
            if (!m_done) {

                if (m_next_ramp == 128) for (int i = 0; i < NUM_LEDS; i++) leds[BACK_LEDS][i] = BLUE;
                leds[BACK_LEDS][NUM_LEDS / 2 - 1 - m_next_ramp] = OFF;
                leds[BACK_LEDS][NUM_LEDS / 2 + m_next_ramp] = OFF;
                m_next_ramp--;
            }
            else return true;
            return false;
        }

        void reset() {

            m_done = false;
            m_next_ramp = 128;
        }

    private:

        bool m_done;
        int m_next_ramp;
};

class BLEDebugOn : public Effect {

    public:

        bool step(CRGB **leds, long current, bool strobeToggle, bool police, bool brake) {

            // First, update timing. If the number of times flashed is odd,
            // then we are off
            if (m_flashed % 2 != 0) {

                if (current - m_timer > 350) {

                    m_flashed++;
                    m_timer = current;
                }
            }
            // If the number of times flashed is even, we are solid blue
            else {

                if (current - m_timer > 250) {

                    m_flashed++;
                    m_timer = current;
                }
            }

            if (m_flashed > 5) m_done = true;
            if (!m_done) {

                if (m_flashed % 2 != 0) {

                    for (int i = 0; i < NUM_LEDS; i++) leds[BACK_LEDS][i] = OFF;
                }
                else {

                    for (int i = 0; i < NUM_LEDS; i++) leds[BACK_LEDS][i] = BLUE;
                }
            }
            else return true;
            return false;
        }

        void reset() {

            m_done = false;
            m_on = true;
            m_flashed = 0;
            m_timer = millis();
        }

    private:

        bool m_done;
        bool m_on;
        int m_flashed;
        long m_timer;
};

class BLEDebugOff : public Effect {

    public:

        bool step(CRGB **leds, long current, bool strobeToggle, bool police, bool brake) {

            // First, update timing. If the number of times flashed is odd,
            // then we are off
            if (m_flashed % 2 != 0) {

                if (current - m_timer > 350) {

                    m_flashed++;
                    m_timer = current;
                }
            }
            // If the number of times flashed is even, we are solid blue
            else {

                if (current - m_timer > 250) {

                    m_flashed++;
                    m_timer = current;
                }
            }

            if (m_flashed > 9) m_done = true;
            if (!m_done) {

                if (m_flashed % 2 != 0) {

                    for (int i = 0; i < NUM_LEDS; i++) leds[BACK_LEDS][i] = OFF;
                }
                else {

                    for (int i = 0; i < NUM_LEDS; i++) leds[BACK_LEDS][i] = BLUE;
                }
            }
            else return true;
            return false;
        }

        void reset() {

            m_done = false;
            m_on = true;
            m_flashed = 0;
            m_timer = millis();
        }

    private:

        bool m_done;
        bool m_on;
        int m_flashed;
        long m_timer;
};

class BLEError : public Effect {

    public:
        
        bool step(CRGB **leds, long current, bool strobeToggle, bool police, bool brake) {

            if (current - m_timer > BLINK_CYCLE_MS / 2) {

                m_on = !m_on;
                m_timer = current;
            }
            if (m_on) for (int i = 0; i < NUM_LEDS; i++) leds[BACK_LEDS][i] = RED;
            else for (int i = 0; i < NUM_LEDS; i++) leds[BACK_LEDS][i] = OFF;
            return false;
        }

        void reset() {

            m_on = true;
            m_timer = millis();
        }

    private:

        bool m_on;
        long m_timer;
};

class BLESearching : public Effect {

    public:

        bool step(CRGB **leds, long current, bool strobeToggle, bool police, bool brake) {

            if (m_next_ramp > NUM_LEDS - 1) {

                m_right = false;
                m_next_ramp = NUM_LEDS - 2;
            }
            else if (m_next_ramp < 0) {

                m_right = true;
                m_next_ramp = 1;
            }
            FastLED.clear();
            leds[BACK_LEDS][m_next_ramp] = BLUE;
            m_next_ramp += m_right ? 1 : -1;
            return false;
        }

        void reset() {

            m_next_ramp = 0;
            m_right = true;
        }

    private:

        int m_next_ramp;
        bool m_right;
};

class HalfHalf : public Effect {

    public:

        bool step(CRGB **leds, long current, bool strobeToggle, bool police, bool brake) {

            if (current - m_timer >= 452) {

                m_right = !m_right;
                m_timer = current;
            }
            for (int j = 0; j < NUM_STRIPS; j++) {
                
                int length = getLength(j);
                for (int i = 0; i < length / 2; i++) {

                    if (!m_right) {

                        leds[j][i] = strobeToggle ? (police ? RED : AMBER) : OFF;
                    }
                    else {

                        leds[j][i] = OFF;
                    }
                }
                for (int i = length / 2; i < length; i++) {

                    if (m_right) {

                        leds[j][i] = strobeToggle ? (police ? BLUE : WHITE) : OFF;
                    }
                    else {

                        leds[j][i] = OFF;
                    }
                }
            }
            return false;
        }
    
        void reset() {

            m_right = false;
            m_timer = millis();
        }

    private:

        bool m_right;
        long m_timer;
};

class Segment : public Effect {

    public:

        bool step(CRGB **leds, long current, bool strobeToggle, bool police, bool brake) {

            if (current - m_timer >= 333) {

                m_red = !m_red;
                m_timer = current;
            }
            
            for (int i = 0; i < NUM_STRIPS; i++) {
                
                int length = getLength(i);
                int batchSize = i == 0 ? 12 : 4;
                int batches = length / batchSize;
                for (int j = 0; j < batches; j++) {
                    
                    int batchSizeHalf = batchSize / 2;
                    for (int k = 0; k < batchSizeHalf; k++) {

                        if (m_red) {

                            leds[i][j * batchSize + k] = strobeToggle ? (police ? BLUE : WHITE) : OFF;
                        }
                        else {

                            leds[i][j * batchSize + k] = OFF;
                        }
                    }
                    for (int k = batchSizeHalf; k < batchSize; k++) {

                        if (!m_red) {

                            leds[i][j * batchSize + k] = strobeToggle ? (police ? RED : AMBER) : OFF;
                        }
                        else {

                            leds[i][j * batchSize + k] = OFF;
                        }
                    }
                }
            }
            leds[LEFT_LEDS][NUM_RB_LEDS - 1] = RED;
            leds[RIGHT_LEDS][NUM_RB_LEDS - 1] = RED;
            return false;
        }

        void reset() {

            m_timer = millis();
            m_red = false;
        }

    private:

        long m_timer;
        bool m_red;
};

class DC17 : public Effect {

    public:

        bool step(CRGB **leds, long current, bool strobeToggle, bool police, bool brake) {

            if (current - m_timer >= 500) {

                if (!m_on) {

                    m_red = !m_red;
                }
                m_on = !m_on;
                m_timer = current;
            }

            for (int i = 0; i < NUM_STRIPS; i++) {
            
                int length = getLength(i);
                for (int j = 0; j < length; j++) {

                    if (m_on) {

                        leds[i][j] = strobeToggle ? (m_red ? (police ? BLUE : WHITE) : (police ? RED : AMBER)) : OFF;
                    }
                    else {

                        leds[i][j] = OFF;
                    }
                }
            }
            return false;
        }

        void reset() {

            m_timer = millis();
            m_red = false;
            m_on = false;
        }

    private:

        long m_timer;
        bool m_red;
        bool m_on;
};

class Director : public Effect {

    public:

        bool step(CRGB **leds, long current, bool strobeToggle, bool police, bool brake) {

            for (int i = 1; i < 3; i++) for (int j = 0; j < NUM_RB_LEDS; j++) leds[i][j] = OFF;
            if (m_ramping) {
                
                if ((current - m_timer >= 5 && (m_direction == 0 || m_direction == 2)) ||
                    (current - m_timer >= 10 && m_direction == 1)) {
                
                    if ((m_next_ramp < 0 && (m_direction == 0 || m_direction == 2)) ||
                        (m_c_next_ramp < 0 && m_direction == 1)) {

                        for (int i = 0; i < NUM_LEDS; i++) leds[BACK_LEDS][i] = OFF;
                        if (m_direction == 0 || m_direction == 2) m_next_ramp = NUM_LEDS - 1;
                        else if (m_direction == 1) m_c_next_ramp = NUM_LEDS / 2 - 1;
                        m_ramping = false;
                    }
                    else {
                        
                        if (m_direction == 0) leds[BACK_LEDS][NUM_LEDS - 1 - m_next_ramp] = AMBER;
                        else if (m_direction == 1) {

                            leds[BACK_LEDS][m_c_next_ramp] = AMBER;
                            leds[BACK_LEDS][NUM_LEDS / 2 + NUM_LEDS / 2 - 1 - m_c_next_ramp] = AMBER;
                        }
                        else if (m_direction == 2) leds[BACK_LEDS][m_next_ramp] = AMBER;
                        m_next_ramp--;
                        m_c_next_ramp--;
                    }
                    m_timer = current;
                }
            }
            else {

                if (m_blinks == 0 || m_blinks == 8) {

                    if (current - m_timer >= 200) {

                        m_blinks++;
                        m_timer = current;
                    }
                    else for (int i = 0; i < NUM_LEDS; i++) leds[BACK_LEDS][i] = OFF;
                }
                else if (m_blinks == 4) {

                    if (current - m_timer >= 400) {

                        m_blinks++;
                        m_timer = current;
                    }
                    else for (int i = 0; i < NUM_LEDS; i++) leds[BACK_LEDS][i] = OFF;
                }
                else if (m_blinks == 1 ||
                         m_blinks == 3 ||
                         m_blinks == 5 ||
                         m_blinks == 7) {

                    if (current - m_timer >= 100) {

                        m_blinks++;
                        m_timer = current;
                    }
                    else for (int i = 0; i < NUM_LEDS; i++) leds[BACK_LEDS][i] = AMBER;
                }
                else {

                    if (current - m_timer >= 150) {

                        m_blinks++;
                        m_timer = current;
                    }
                    else for (int i = 0; i < NUM_LEDS; i++) leds[BACK_LEDS][i] = OFF;
                }
                if (m_blinks > 8) {

                    m_blinks = 0;
                    m_ramping = true;
                }
            }
            return false;
        }

        void reset() {

            m_timer = millis();
            m_ramping = true;
            m_next_ramp = NUM_LEDS - 1;
            m_c_next_ramp = NUM_LEDS / 2 - 1;
            m_blinks = 0;
        }

        int m_direction;

    private:

        long m_timer;
        bool m_ramping;
        int m_next_ramp;
        int m_c_next_ramp;
        int m_blinks;
};

class RainbowWave : public Effect {

    public:

        bool step(CRGB **leds, long current, bool strobeToggle, bool police, bool brake) {

            for (int i = 45; i > 0; i--) {

                leds[LEFT_LEDS][i] = CHSV((((45 - i) * 3) * 255.0 / (366)) + m_waveCounter * WAVE_SPEED_SCALAR, 255, 255);
            }
            for (int i = 0; i < NUM_LEDS; i++) {

                leds[BACK_LEDS][i] = CHSV(((i + (45 * 3)) * 255.0 / (366)) + m_waveCounter * WAVE_SPEED_SCALAR, 255, 255);
            }
            for (int i = 0; i < 45; i++) {

                leds[RIGHT_LEDS][i] = CHSV((((i * 3) + (45 * 3) + NUM_LEDS) * 255.0 / (366)) + m_waveCounter * WAVE_SPEED_SCALAR, 255, 255);
            }
            m_waveCounter += (255.0 / (double)(366)) * WAVE_SPEED_SCALAR;
            return false;
        }

        void reset() {

            m_waveCounter = 0.0;
        }

    private:

        double m_waveCounter;
};

class Music : public Effect {

    public:

        bool step(CRGB **leds, long current, bool strobeToggle, bool police, bool brake) {}
        void reset() {}
};

class Trail : public Effect {

    public:

        bool step(CRGB **leds, long current, bool strobeToggle, bool police, bool brake) {}
        void reset() {}
};

class Solid : public Effect {

    public:

        bool step(CRGB **leds, long current, bool strobeToggle, bool police, bool brake) {}
        void reset() {}
};

class Blinker : public Effect {

    public:

        bool step(CRGB **leds, long current, bool strobeToggle, bool police, bool brake) {

            for (int i = 1; i < 3; i++) for (int j = 0; j < NUM_RB_LEDS; j++) leds[i][j] = OFF;
            if (current - m_timer >= BLINK_MAX_OFF / NUM_LEDS / 2) {

                if (m_next_ramp >= 0) {
                    
                    if (m_direction == 0) leds[BACK_LEDS][m_next_ramp] = AMBER;
                    else if (m_direction == 1) leds[BACK_LEDS][NUM_LEDS / 2 + NUM_LEDS / 2 - 1 - m_next_ramp] = AMBER;
                    m_next_ramp--;
                }
                m_timer = current;
            }
            if (m_direction == 0) for (int i = NUM_LEDS / 2; i < NUM_LEDS; i++) leds[BACK_LEDS][i] = brake ? RED : OFF;
            else if (m_direction == 1) for (int i = 0; i < NUM_LEDS / 2; i++) leds[BACK_LEDS][i] = brake ? RED : OFF;
            return false;
        }

        bool m_ramp_done;
        bool m_was_on_off_cycle;
        bool m_on_off_cycle;
        int m_direction;
        long m_last_left;
        long m_last_right;
        bool m_right_blinking;
        bool m_left_blinking;

        void reset() {

            m_timer = millis();
            m_next_ramp = NUM_LEDS / 2 - 1;
        }

    private:

        long m_timer;
        int m_next_ramp;
};

class LeftOff : public Effect {

    public:

        bool step(CRGB **leds, long current, bool strobeToggle, bool police, bool brake) {

            for (int i = 0; i < NUM_LEDS / 2; i++) leds[BACK_LEDS][i] = OFF;
            return false;
        }

        void reset() {}
};

class RightOff : public Effect {

    public:

        bool step(CRGB **leds, long current, bool strobeToggle, bool police, bool brake) {

            for (int i = NUM_LEDS / 2; i < NUM_LEDS; i++) leds[BACK_LEDS][i] = OFF;
            return false;
        }

        void reset() {}
};

class Brake : public Effect {

    public:

        bool step(CRGB **leds, long current, bool strobeToggle, bool police, bool brake) {

            for (int i = 1; i < 3; i++) for (int j = 0; j < NUM_RB_LEDS; j++) leds[i][j] = OFF;
            for (int i = 0; i < NUM_LEDS; i++) leds[BACK_LEDS][i] = RED;
            return false;
        }

        void reset() {}
};

class Backup : public Effect {

    public:

        bool step(CRGB **leds, long current, bool strobeToggle, bool police, bool brake) {

            for (int i = 0; i < NUM_STRIPS; i++) {

                int length = getLength(i);
                for (int j = 0; j < NUM_LEDS; j++) leds[i][j] = WHITE;
            }
            return false;
        }

        void reset() {}
};