#include <Arduino.h>
#include "config.h"
#include "state_machine.h"
#include "motor.h"
#include "ramp.h"

static Zone previousZone = ZONE_DEAD;

static int leftPower = 0;
static int rightPower = 0;

void updateState(Zone zone) {

    // ===== смена направления =====
    if (zone != previousZone) {
        leftPower = 0;
        rightPower = 0;
        setLeftMotor(0);
        setRightMotor(0);
    }

    switch (zone) {

        case ZONE_LEFT:
            rightPower = rampUp(rightPower);
            leftPower = 0;
            break;

        case ZONE_RIGHT:
            leftPower = rampUp(leftPower);
            rightPower = 0;
            break;

        case ZONE_DEAD:
        default:
            leftPower = 0;
            rightPower = 0;
            break;
    }

    setLeftMotor(leftPower);
    setRightMotor(rightPower);

    previousZone = zone;
}