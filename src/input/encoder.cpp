#include <Arduino.h>
#include "input/encoder.h"
#include "config/settings.h"

namespace encoder {

    // Blank namespace keeps helper functions and local variables private to this file
    namespace {
        const int kEncoderPinA = 12;
        const int kEncoderPinB = 11; 
        const int kEncoderSwitch = 10;

        volatile int enc_delta = 0;
        volatile int enc_accumulator = 0;
        volatile int previous = 0;

        const int8_t decoder[16] {
        0, -1, +1,  0,
        +1,  0,  0, -1,
        -1,  0,  0, +1,
        0, +1, -1,  0
        };
        void encoderSwitch() {
            static unsigned long last_pressed = 0;
            if (millis() - last_pressed > 200) {
                flags::menu_btn_flag = true;
            }
            last_pressed = millis();
        }

        void encoderISR() {
            uint8_t current = (digitalRead(kEncoderPinA) << 1) | digitalRead(kEncoderPinB);
            uint8_t index = (previous << 2) | current;
            int8_t step = decoder[index];

            Serial.print("Current state: ");
            Serial.print(current, BIN);
            Serial.print("  |  Previous state: ");
            Serial.print(previous, BIN);
            Serial.print("  |  Transition value: ");
            Serial.println(step, DEC);

            if (step) {
                enc_accumulator += step;
                
                if (enc_accumulator >= 2) {enc_delta++; enc_accumulator = 0;}
                else if (enc_accumulator <= -2) {enc_delta--; enc_accumulator = 0;}
            }

            previous = current;
        }
    }


    void initEncoder() {
        pinMode(kEncoderPinA, INPUT_PULLUP);
        pinMode(kEncoderPinB, INPUT_PULLUP);
        pinMode(kEncoderSwitch, INPUT_PULLUP);

        previous = (digitalRead(kEncoderPinA) << 1) | digitalRead(kEncoderPinB);

        attachInterrupt(digitalPinToInterrupt(kEncoderPinA), encoderISR, CHANGE);
        attachInterrupt(digitalPinToInterrupt(kEncoderPinB), encoderISR, CHANGE);
        attachInterrupt(digitalPinToInterrupt(kEncoderSwitch), encoderSwitch, FALLING);
    }

    int getEncoderDelta() {
        noInterrupts();
        if (enc_delta > 0) {
            Serial.println(enc_delta);
        }
        int steps = enc_delta;
        enc_delta = 0; 
        interrupts();
        return steps;
    }

}