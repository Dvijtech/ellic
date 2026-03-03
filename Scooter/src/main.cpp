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
    analogSetPinAttenuation(PIN_JOYSTICK_X, ADC_11db); // Настройка АЦП для джойстика (полный диапазон 0–3.3В)
    joystickInit();
    motorInit();
}

void loop() {

    Serial.println(analogRead(PIN_JOYSTICK_X));
    delay(200);

    if (millis() - lastUpdate >= LOOP_PERIOD_MS) {
        lastUpdate = millis();

        Zone zone = readJoystick();
        updateState(zone);
    }
}