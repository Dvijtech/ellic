#pragma once
#include <Arduino.h>
#include "Config.h"

// MotionController: расчёт ValDelta, логика тормозов/поворота,
// вычисление leftWheelDelta / rightWheelDelta.
// Вызывается раз в CONTROL_PERIOD_MS (раздел 6.2, 7, 8 спецификации).
class MotionController {
public:
    MotionController();

    void begin();

    // rawAngleDeg      - текущий Val (0..360), для проверки TURN_ZONE
    // continuousAngleDeg - текущий continuous angle, для ValDelta
    // leftBrake/rightBrake - true, если тормоз нажат (digitalRead == LOW)
    void update(float rawAngleDeg, float continuousAngleDeg,
                bool leftBrake, bool rightBrake);

    float getLeftWheelDelta() const  { return _leftWheelDelta; }
    float getRightWheelDelta() const { return _rightWheelDelta; }

    MotionSnapshot getSnapshot() const;

private:
    static bool isInTurnZone(float angleDeg);

    bool  _initialized;
    float _previousContinuousAngle;

    float _valDelta;
    float _leftWheelDelta;
    float _rightWheelDelta;

    bool _leftBrake;
    bool _rightBrake;
    bool _inTurnZone;
    MotionMode _mode;
};
