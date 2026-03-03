#include <Arduino.h>
#include "config.h"
#include "joystick.h"

static int center = 2048;

void joystickInit() {
    pinMode(PIN_JOYSTICK_X, INPUT);

    delay(500); // стабилизация

    // авто-калибровка центра
    long sum = 0;
    for (int i = 0; i < 100; i++) {
        sum += analogRead(PIN_JOYSTICK_X);
        delay(5);
    }
    center = sum / 100;

    Serial.print("Joystick center: ");
    Serial.println(center);
}

Zone readJoystick() {

    int value = analogRead(PIN_JOYSTICK_X);

    int threshold = (Deadzone / 100.0) * 4095.0;

    if (value < center - threshold)
        return ZONE_LEFT;

    if (value > center + threshold)
        return ZONE_RIGHT;

    return ZONE_DEAD;
}