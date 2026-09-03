#include "MotionController.h"
#include "ODriveUART.h"
#include "Telemetry.h"

MotionController::MotionController(Encoder &encoder, ODriveUART &leftOdrive, ODriveUART &rightOdrive)
: _encoder(encoder), _leftOdrive(leftOdrive), _rightOdrive(rightOdrive), _telemetry(nullptr),
  _prevContinuousAngle(0.0f)
{
    _lastSnapshot.valDelta = 0.0f;
    _lastSnapshot.leftWheelDelta = 0.0f;
    _lastSnapshot.rightWheelDelta = 0.0f;
    _lastSnapshot.mode = MotionSnapshot::Mode::CALM;
    _lastSnapshot.leftBrake = false;
    _lastSnapshot.rightBrake = false;
    _lastSnapshot.inTurnZone = false;
}

void MotionController::setTelemetry(Telemetry* telemetry) {
    _telemetry = telemetry;
}

void MotionController::begin() {
    pinMode(LEFT_BRAKE_PIN, INPUT_PULLUP);
    pinMode(RIGHT_BRAKE_PIN, INPUT_PULLUP);
    _prevContinuousAngle = _encoder.getContinuousAngle();
}

bool MotionController::isInTurnZone(float rawAngleDeg) const {
    float a = rawAngleDeg;
    while (a < 0.0f)    a += 360.0f;
    while (a >= 360.0f) a -= 360.0f;

    bool nearZero = (a >= (360.0f - TURN_ZONE_DEG)) || (a <= TURN_ZONE_DEG);
    bool near180  = (a >= (180.0f - TURN_ZONE_DEG)) && (a <= (180.0f + TURN_ZONE_DEG));
    return nearZero || near180;
}

void MotionController::update() {
    float nowAngle = _encoder.getContinuousAngle();
    float valDelta = nowAngle - _prevContinuousAngle;
    _prevContinuousAngle = nowAngle;

    bool leftBrake  = (digitalRead(LEFT_BRAKE_PIN)  == LOW);
    bool rightBrake = (digitalRead(RIGHT_BRAKE_PIN) == LOW);

    bool inTurnZone = isInTurnZone(_encoder.getRawAngle());

    float leftWheelDelta = 0.0f;
    float rightWheelDelta = 0.0f;
    MotionSnapshot::Mode mode;

    // Порядок проверок фиксирован спецификацией (раздел 8):
    if (leftBrake && rightBrake) {
        leftWheelDelta = 0.0f;
        rightWheelDelta = 0.0f;
        mode = MotionSnapshot::Mode::CALM;
    } else if (!leftBrake && !rightBrake) {
        leftWheelDelta  = valDelta * MOTOR_GEAR_RATIO / 360.0f * LEFT_WHEEL_SIGN;
        rightWheelDelta = valDelta * MOTOR_GEAR_RATIO / 360.0f * RIGHT_WHEEL_SIGN;
        mode = MotionSnapshot::Mode::NORMAL;
    } else if (leftBrake && !rightBrake) {
        if (inTurnZone) {
            leftWheelDelta  = 0.0f;
            rightWheelDelta = TURN_STEP * RIGHT_WHEEL_SIGN;
            mode = MotionSnapshot::Mode::TURN;
        } else {
            leftWheelDelta  = 0.0f;
            rightWheelDelta = 0.0f;
            mode = MotionSnapshot::Mode::CALM;
        }
    } else { // rightBrake && !leftBrake
        if (inTurnZone) {
            leftWheelDelta  = TURN_STEP * LEFT_WHEEL_SIGN;
            rightWheelDelta = 0.0f;
            mode = MotionSnapshot::Mode::TURN;
        } else {
            leftWheelDelta  = 0.0f;
            rightWheelDelta = 0.0f;
            mode = MotionSnapshot::Mode::CALM;
        }
    }

    _leftOdrive.requestMove(leftWheelDelta);
    _rightOdrive.requestMove(rightWheelDelta);

    _lastSnapshot.valDelta = valDelta;
    _lastSnapshot.leftWheelDelta = leftWheelDelta;
    _lastSnapshot.rightWheelDelta = rightWheelDelta;
    _lastSnapshot.mode = mode;
    _lastSnapshot.leftBrake = leftBrake;
    _lastSnapshot.rightBrake = rightBrake;
    _lastSnapshot.inTurnZone = inTurnZone;
}

MotionSnapshot MotionController::getSnapshot() const {
    return _lastSnapshot;
}