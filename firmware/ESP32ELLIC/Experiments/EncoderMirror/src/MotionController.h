#pragma once

#include <Arduino.h>
#include "Encoder.h"
#include "ODriveUART.h"
#include "PhaseDetector.h"

// Основная логика: держать позицию по валу, обрабатывать ручной доворот на тормозах
class MotionController
{
public:
    MotionController(Encoder &encoder,
                      ODriveUART &leftDrive,
                      ODriveUART &rightDrive,
                      PhaseDetector &phase,
                      float gearRatio,
                      float turnStep,
                      uint32_t controlPeriodMs);

    void begin();

    // Сама решает, когда пора считать очередной шаг (по controlPeriodMs)
    void update();

    // ---- для отладочного вывода ----
    float targetTurns()     const { return _targetTurnsV; }
    float leftHoldOffset()  const { return _leftHoldOffsetW; }
    float rightHoldOffset() const { return _rightHoldOffsetW; }
    bool  isTurning()       const { return _turning; }

private:
    Encoder &_encoder;
    ODriveUART &_left;
    ODriveUART &_right;
    PhaseDetector &_phase;

    float _gearRatio;
    float _turnStep;
    uint32_t _controlPeriodMs;

    unsigned long _lastControl = 0;

    float _targetTurnsV = 0;

    float _leftTurnOffsetW = 0;
    float _rightTurnOffsetW = 0;

    float _leftHoldOffsetW = 0;
    float _rightHoldOffsetW = 0;

    bool _turning = false;

    void finishTurn();
};