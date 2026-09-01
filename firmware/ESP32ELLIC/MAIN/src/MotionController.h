#pragma once

#include <Arduino.h>
#include "Encoder.h"
#include "ODriveUART.h"

class MotionController
{
public:

    MotionController(
        Encoder &encoder,
        ODriveUART &leftODrive,
        ODriveUART &rightODrive
    );

    void begin(
        int leftBrakePin,
        int rightBrakePin
    );

    void update();

private:

    Encoder &_encoder;

    ODriveUART &_leftODrive;
    ODriveUART &_rightODrive;

    int _leftBrakePin = -1;
    int _rightBrakePin = -1;

    float _previousValContinuousAngle = 0.0f;

    bool _initialized = false;

    static constexpr float MOTOR_GEAR_RATIO = 4.4f;

    static constexpr float TURN_ZONE_DEG = 5.0f;
    static constexpr float TURN_STEP = 0.03f;

    static constexpr float LEFT_WHEEL_SIGN = +1.0f;
    static constexpr float RIGHT_WHEEL_SIGN = -1.0f;

    bool leftBrakePressed() const;
    bool rightBrakePressed() const;

    bool valInTurnZone(float valAngle) const;

    float calculateValDelta();

    void processNormalMotion(float valDelta);

    void processLeftBrake();
    void processRightBrake();

    void calm();

    void moveLeftWheel(float wheelDelta);
    void moveRightWheel(float wheelDelta);
};