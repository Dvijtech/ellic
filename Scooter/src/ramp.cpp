#include <Arduino.h>
#include "config.h"
#include "ramp.h"

int rampUp(int current) {

    if (current >= MaxPower)
        return MaxPower;

    current += RampStep;

    if (current > MaxPower)
        current = MaxPower;

    return current;
}