#pragma once

#include <Arduino.h>

#include "Encoder.h"
#include "ODriveUART.h"
#include "PhaseDetector.h"

// ============================================================
// MotionController
//
// Два взаимоисключающих режима:
//
// WALKING
//     Колёса следуют за вращением коленвала.
//
// TURNING
//     Коленвал находится в turn zone,
//     один тормоз нажат,
//     соответствующее колесо получает дополнительное движение.
//
// WALKING и TURNING одновременно невозможны.
// ============================================================

class MotionController
{
public:

    MotionController(
        Encoder &encoder,
        ODriveUART &leftDrive,
        ODriveUART &rightDrive,
        PhaseDetector &phase,
        float gearRatio,
        float turnStep,
        uint32_t controlPeriodMs);

    void begin();

    // Вызывать каждый loop()
    void update();

    // --------------------------------------------------------
    // DEBUG
    // --------------------------------------------------------

    float leftTarget() const
    {
        return _leftTarget;
    }

    float rightTarget() const
    {
        return _rightTarget;
    }

    float leftTurnOffset() const
    {
        return _leftTurnOffset;
    }

    float rightTurnOffset() const
    {
        return _rightTurnOffset;
    }

    float valSpeedDegS() const
    {
        return _valSpeedDegS;
    }

    bool isWalking() const
    {
        return _walking;
    }

    bool isTurning() const
    {
        return _turning;
    }

    bool isStopped() const
    {
        return _stopped;
    }

private:

    // ========================================================
    // HARDWARE
    // ========================================================

    Encoder &_encoder;

    ODriveUART &_left;
    ODriveUART &_right;

    PhaseDetector &_phase;

    // ========================================================
    // PARAMETERS
    // ========================================================

    float _gearRatio;
    float _turnStep;

    uint32_t _controlPeriodMs;

    // Вал считается неподвижным,
    // если его скорость <= 3 градусов/сек.
    static constexpr float VAL_STOP_SPEED_DEG_S = 3.0f;

    // ========================================================
    // BRAKES
    // ========================================================

    static constexpr int LEFT_BRAKE_PIN = 32;
    static constexpr int RIGHT_BRAKE_PIN = 33;

    // ========================================================
    // TIMING
    // ========================================================

    uint32_t _lastControl = 0;

    uint32_t _lastValSampleTime = 0;

    // ========================================================
    // VAL MOVEMENT
    // ========================================================

    float _previousValAngle = 0.0f;

    float _valSpeedDegS = 0.0f;

    bool _walking = false;

    bool _stopped = true;

    bool _turning = false;

    // ========================================================
    // COMMAND BASE
    // ========================================================
    
    // Командная позиция колёс в момент,
    // когда начинается новый цикл движения вала.
    //
    // Это НЕ измеренная физическая позиция колеса.
    // Это математическая база команд.

    float _leftBaseTarget = 0.0f;

    float _rightBaseTarget = 0.0f;

    // Непрерывная координата вала
    // в момент создания базы.
    float _valBaseAngle = 0.0f;

    // ========================================================
    // TURN OFFSETS
    // ========================================================

    float _leftTurnOffset = 0.0f;

    float _rightTurnOffset = 0.0f;

    // ========================================================
    // CURRENT TARGETS
    // ========================================================

    float _leftTarget = 0.0f;

    float _rightTarget = 0.0f;

    // ========================================================
    // INTERNAL
    // ========================================================

    void updateValSpeed();

    void updateState();

    void updateWalking();

    void updateTurning();

    void createNewBase();

    void resetTurnOffsets();

    void readBrakes(
        bool &leftBrake,
        bool &rightBrake) const;
};