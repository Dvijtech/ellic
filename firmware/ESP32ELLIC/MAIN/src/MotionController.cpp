#include "MotionController.h"

MotionController::MotionController(
    Encoder &encoder,
    ODriveUART &leftODrive,
    ODriveUART &rightODrive
)
    : _encoder(encoder),
      _leftODrive(leftODrive),
      _rightODrive(rightODrive)
{
}

void MotionController::begin(
    int leftBrakePin,
    int rightBrakePin
)
{
    _leftBrakePin = leftBrakePin;
    _rightBrakePin = rightBrakePin;

    pinMode(
        _leftBrakePin,
        INPUT_PULLUP
    );

    pinMode(
        _rightBrakePin,
        INPUT_PULLUP
    );

    _previousValContinuousAngle =
        _encoder.valContinuousAngle();

    _initialized = true;
}

void MotionController::update()
{
    if (!_initialized)
        return;

    float valDelta =
        calculateValDelta();

    bool leftBrake =
        leftBrakePressed();

    bool rightBrake =
        rightBrakePressed();

    bool anyBrake =
        leftBrake || rightBrake;

    bool bothBrakes =
        leftBrake && rightBrake;

    // --------------------------------------------------------
    // BOTH BRAKES
    // --------------------------------------------------------

    if (bothBrakes)
    {
        calm();
        return;
    }

    // --------------------------------------------------------
    // NO BRAKES
    // --------------------------------------------------------

    if (!anyBrake)
    {
        processNormalMotion(valDelta);
        return;
    }

    // --------------------------------------------------------
    // LEFT BRAKE
    // --------------------------------------------------------

    if (leftBrake)
    {
        processLeftBrake();
        return;
    }

    // --------------------------------------------------------
    // RIGHT BRAKE
    // --------------------------------------------------------

    if (rightBrake)
    {
        processRightBrake();
        return;
    }

    calm();
}

bool MotionController::leftBrakePressed() const
{
    return digitalRead(_leftBrakePin) == LOW;
}

bool MotionController::rightBrakePressed() const
{
    return digitalRead(_rightBrakePin) == LOW;
}

float MotionController::calculateValDelta()
{
    float current =
        _encoder.valContinuousAngle();

    float delta =
        current - _previousValContinuousAngle;

    _previousValContinuousAngle =
        current;

    return delta;
}

bool MotionController::valInTurnZone(
    float valAngle
) const
{
    while (valAngle >= 360.0f)
        valAngle -= 360.0f;

    while (valAngle < 0.0f)
        valAngle += 360.0f;

    bool zoneAroundZero =
        valAngle >=
            (360.0f - TURN_ZONE_DEG)
        ||
        valAngle <= TURN_ZONE_DEG;

    bool zoneAround180 =
        valAngle >=
            (180.0f - TURN_ZONE_DEG)
        &&
        valAngle <=
            (180.0f + TURN_ZONE_DEG);

    return zoneAroundZero || zoneAround180;
}

void MotionController::processNormalMotion(
    float valDelta
)
{
    float leftWheelDelta =
        valDelta *
        MOTOR_GEAR_RATIO /
        360.0f *
        LEFT_WHEEL_SIGN;

    float rightWheelDelta =
        valDelta *
        MOTOR_GEAR_RATIO /
        360.0f *
        RIGHT_WHEEL_SIGN;

    moveLeftWheel(leftWheelDelta);
    moveRightWheel(rightWheelDelta);
}

void MotionController::processLeftBrake()
{
    float valAngle =
        _encoder.valRawAngle();

    if (!valInTurnZone(valAngle))
    {
        calm();
        return;
    }

    moveLeftWheel(0.0f);

    moveRightWheel(
        TURN_STEP *
        RIGHT_WHEEL_SIGN
    );
}

void MotionController::processRightBrake()
{
    float valAngle =
        _encoder.valRawAngle();

    if (!valInTurnZone(valAngle))
    {
        calm();
        return;
    }

    moveLeftWheel(
        TURN_STEP *
        LEFT_WHEEL_SIGN
    );

    moveRightWheel(0.0f);
}

void MotionController::calm()
{
    moveLeftWheel(0.0f);
    moveRightWheel(0.0f);
}

void MotionController::moveLeftWheel(
    float wheelDelta
)
{
    float currentPosition;

    if (!_leftODrive.getPosition(
        currentPosition
    ))
    {
        return;
    }

    float newPosition =
        currentPosition +
        wheelDelta;

    _leftODrive.sendPosition(
        newPosition
    );
}

void MotionController::moveRightWheel(
    float wheelDelta
)
{
    float currentPosition;

    if (!_rightODrive.getPosition(
        currentPosition
    ))
    {
        return;
    }

    float newPosition =
        currentPosition +
        wheelDelta;

    _rightODrive.sendPosition(
        newPosition
    );
}