#include "MotionController.h"
#include "Telemetry.h"

MotionController::MotionController(Encoder& encoder, ODriveCAN& odrive, Telemetry* telemetry)
    : encoder_(encoder),
      odrive_(odrive),
      telemetry_(telemetry),
      lastControlMs_(0),
      controlInitialized_(false),
      previousControlAngle_(0.0f),
      snapshot_{} {
    snapshot_.mode = MotionSnapshot::Mode::CALM;
}

void MotionController::begin() {
    pinMode(LEFT_BRAKE_PIN, INPUT_PULLUP);
    pinMode(RIGHT_BRAKE_PIN, INPUT_PULLUP);
    previousControlAngle_ = encoder_.getContinuousAngle();
    lastControlMs_ = millis();
    controlInitialized_ = true;
    setCalm();
}

void MotionController::update() {
    const uint32_t now = millis();
    if (!controlInitialized_) {
        begin();
    }

    if (static_cast<uint32_t>(now - lastControlMs_) < CONTROL_PERIOD_MS) {
        return;
    }
    lastControlMs_ = now;

    const float continuousNow = encoder_.getContinuousAngle();
    const float valDelta = continuousNow - previousControlAngle_;
    previousControlAngle_ = continuousNow;

    const bool leftBrake = digitalRead(LEFT_BRAKE_PIN) == LOW;
    const bool rightBrake = digitalRead(RIGHT_BRAKE_PIN) == LOW;
    const float rawAngle = encoder_.getRawAngle();
    const bool turnZone = isTurnZone(rawAngle);

    snapshot_.valDelta = valDelta;
    snapshot_.leftBrake = leftBrake;
    snapshot_.rightBrake = rightBrake;
    snapshot_.inTurnZone = turnZone;

    if (leftBrake && rightBrake) {
        setCalm();
    } else if (!leftBrake && !rightBrake) {
        setNormal(valDelta);
    } else if (leftBrake && !rightBrake) {
        processLeftBrake(rawAngle);
    } else {
        processRightBrake(rawAngle);
    }

    // Each wheel is handled independently. A missing/stale position only suppresses
    // the command for that wheel; the other channel remains untouched.
    {
        if (!odrive_.moveLeft(snapshot_.leftWheelDelta) && telemetry_) {
            telemetry_->log(LogLevel::WARNING, "MotionController", "LEFT Set Input Pos skipped: position unavailable");
        }
    }

    {
        if (!odrive_.moveRight(snapshot_.rightWheelDelta) && telemetry_) {
            telemetry_->log(LogLevel::WARNING, "MotionController", "RIGHT Set Input Pos skipped: position unavailable");
        }
    }
}

bool MotionController::isTurnZone(float rawAngle) const {
    if (rawAngle >= 180.0f - TURN_ZONE_DEG && rawAngle <= 180.0f + TURN_ZONE_DEG) {
        return true;
    }
    return rawAngle >= 360.0f - TURN_ZONE_DEG || rawAngle <= TURN_ZONE_DEG;
}

void MotionController::setCalm() {
    snapshot_.mode = MotionSnapshot::Mode::CALM;
    snapshot_.leftWheelDelta = 0.0f;
    snapshot_.rightWheelDelta = 0.0f;
}

void MotionController::setNormal(float valDelta) {
    snapshot_.mode = MotionSnapshot::Mode::NORMAL;
    snapshot_.leftWheelDelta = valDelta * MOTOR_GEAR_RATIO / 360.0f * LEFT_WHEEL_SIGN;
    snapshot_.rightWheelDelta = valDelta * MOTOR_GEAR_RATIO / 360.0f * RIGHT_WHEEL_SIGN;
}

void MotionController::processLeftBrake(float rawAngle) {
    if (!isTurnZone(rawAngle)) {
        setCalm();
        return;
    }

    snapshot_.mode = MotionSnapshot::Mode::TURN;
    snapshot_.leftWheelDelta = 0.0f;
    snapshot_.rightWheelDelta = TURN_STEP * RIGHT_WHEEL_SIGN;
}

void MotionController::processRightBrake(float rawAngle) {
    if (!isTurnZone(rawAngle)) {
        setCalm();
        return;
    }

    snapshot_.mode = MotionSnapshot::Mode::TURN;
    snapshot_.leftWheelDelta = TURN_STEP * LEFT_WHEEL_SIGN;
    snapshot_.rightWheelDelta = 0.0f;
}

MotionSnapshot MotionController::getSnapshot() const {
    return snapshot_;
}
