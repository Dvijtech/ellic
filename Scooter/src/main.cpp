#include <Arduino.h>
#include "config.h"
#include "joystick.h"
#include "motor.h"
#include "state_machine.h"

// ===== Global parameters =====
int MaxPower = 60;
int RampStep = 2;
int Deadzone = 30;
int Mode = 0;

unsigned long lastUpdate = 0;

void setup() {
    Serial.begin(115200);

    joystickInit();
    motorInit();
}

void loop() {

    if (millis() - lastUpdate >= LOOP_PERIOD_MS) {
        lastUpdate = millis();

        Zone zone = readJoystick();
        updateState(zone);
    }
}