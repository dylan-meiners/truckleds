#include <Arduino.h>

char buffer[4];

void setup() {

    Serial.begin(115200);
}

void loop() {

    char data[128] = { 0 };
    if (Serial.available() > 0) {
    
        int read = Serial.readBytes(data, 128);
        itoa(read, buffer, 10);
        Serial.print(buffer); Serial.print(": "); Serial.println(data[127]);
    }
}