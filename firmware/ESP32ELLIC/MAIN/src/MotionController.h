#ifndef MOTION_CONTROLLER_H
#define MOTION_CONTROLLER_H

#include <Arduino.h>
#include "Encoder.h"

class Telemetry;
class ODriveUART;

struct MotionSnapshot {
    float valDelta;
    float leftWheelDelta;
    float rightWheelDelta;
    enum class Mode { NORMAL, CALM, TURN } mode;
    bool leftBrake;
    bool rightBrake;
    bool inTurnZone;
};

class MotionController {
public:
    static const uint32_t CONTROL_PERIOD_MS   = 300;
    static constexpr float MOTOR_GEAR_RATIO   = 4.4f;
    static constexpr float TURN_ZONE_DEG      = 10.0f;
    static constexpr float TURN_STEP          = 0.03f;
    static constexpr float LEFT_WHEEL_SIGN    = 1.0f;
    static constexpr float RIGHT_WHEEL_SIGN   = -1.0f;

    static const int LEFT_BRAKE_PIN  = 32;
    static const int RIGHT_BRAKE_PIN = 33;

    MotionController(Encoder &encoder, ODriveUART &leftOdrive, ODriveUART &rightOdrive);

    void setTelemetry(Telemetry* telemetry);

    void begin();
    void update(); // вызывается раз в CONTROL_PERIOD_MS

    MotionSnapshot getSnapshot() const;

private:
    Encoder &_encoder;
    ODriveUART &_leftOdrive;
    ODriveUART &_rightOdrive;
    Telemetry* _telemetry;

    float _prevContinuousAngle;

    MotionSnapshot _lastSnapshot;

    bool isInTurnZone(float rawAngleDeg) const;
};

#endif