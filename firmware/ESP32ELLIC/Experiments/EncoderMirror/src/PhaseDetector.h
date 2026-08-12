#pragma once

#include <Arduino.h>

// Тормозные ручки + определение "зоны поворота" по углу вала
class PhaseDetector
{
public:
    void begin(int leftBrakePin, int rightBrakePin);

    // Вызывать в каждой итерации loop()
    void update();

    bool leftBrake()  const { return _leftBrake; }
    bool rightBrake() const { return _rightBrake; }

    static bool inTurnZone(float angleDeg);

private:
    int _leftPin = -1;
    int _rightPin = -1;

    bool _leftBrake = false;
    bool _rightBrake = false;
};