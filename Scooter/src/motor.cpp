#include <Arduino.h>
#include "config.h"
#include "motor.h"

void motorInit() {
// удаляем тк удалили пины направлений
   // pinMode(PIN_DIR_LEFT, OUTPUT);
    //pinMode(PIN_DIR_RIGHT, OUTPUT);

    //digitalWrite(PIN_DIR_LEFT, HIGH);
    //digitalWrite(PIN_DIR_RIGHT, HIGH);

    ledcSetup(0, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PIN_PWM_LEFT, 0);

    ledcSetup(1, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PIN_PWM_RIGHT, 1);
}

static int percentToPWM(int percent) {
    percent = constrain(percent, 0, 100);
    return map(percent, 0, 100, 0, PWM_MAX);
}

void setLeftMotor(int percent) {
    ledcWrite(0, percentToPWM(percent));
}

void setRightMotor(int percent) {
    ledcWrite(1, percentToPWM(percent));
}