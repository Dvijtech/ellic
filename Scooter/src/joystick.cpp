#include <Arduino.h>
#include "config.h"
#include "joystick.h"

void joystickInit() {
    pinMode(PIN_JOYSTICK_X, INPUT);
}

Zone readJoystick() {
    int raw = analogRead(PIN_JOYSTICK_X);
    float percent = (raw / 4095.0) * 100.0;

    if (percent < Deadzone) return ZONE_LEFT;
    if (percent > (100 - Deadzone)) return ZONE_RIGHT;

    return ZONE_DEAD;
}