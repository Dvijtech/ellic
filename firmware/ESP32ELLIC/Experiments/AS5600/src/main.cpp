#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    delay(3000);

    Serial.println();
    Serial.println("===== START =====");
}

void loop() {
    Serial.println("ESP32 OK");
    delay(1000);
}