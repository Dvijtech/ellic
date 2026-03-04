#include <Arduino.h>
#include "config.h"
#include "motor.h"

void motorInit() {


    ledcSetup(0, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(1, PWM_FREQ, PWM_RESOLUTION);

    ledcAttachPin(PIN_PWM_LEFT, 0);
    ledcAttachPin(PIN_PWM_RIGHT, 1);

    // ВАЖНО: сразу после attach
    ledcWrite(0, 0);
    ledcWrite(1, 0);
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