#include "PhaseDetector.h"

void PhaseDetector::begin(int leftBrakePin, int rightBrakePin)
{
    _leftPin = leftBrakePin;
    _rightPin = rightBrakePin;

    pinMode(_leftPin, INPUT_PULLDOWN);
    pinMode(_rightPin, INPUT_PULLDOWN);
}

void PhaseDetector::update()
{
    _leftBrake = digitalRead(_leftPin);
    _rightBrake = digitalRead(_rightPin);
}

bool PhaseDetector::inTurnZone(float angleDeg)
{
    if (angleDeg >= 350 || angleDeg <= 10) return true;
    if (angleDeg >= 170 && angleDeg <= 190) return true;

    return false;
}