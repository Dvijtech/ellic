#include "MotionController.h"
#include <math.h>

MotionController::MotionController()
    : _initialized(false),
      _previousContinuousAngle(0.0f),
      _valDelta(0.0f),
      _leftWheelDelta(0.0f),
      _rightWheelDelta(0.0f),
      _leftBrake(false),
      _rightBrake(false),
      _inTurnZone(false),
      _mode(MotionMode::CALM) {}

void MotionController::begin() {
    _initialized = false;
    _previousContinuousAngle = 0.0f;
    _valDelta = 0.0f;
    _leftWheelDelta = 0.0f;
    _rightWheelDelta = 0.0f;
    _leftBrake = false;
    _rightBrake = false;
    _inTurnZone = false;
    _mode = MotionMode::CALM;
}

// Раздел 8.1: зоны 170..190 и 350..360/0..10 при TURN_ZONE_DEG = 10.0
bool MotionController::isInTurnZone(float angleDeg) {
    float a = fmodf(angleDeg, 360.0f);
    if (a < 0.0f) a += 360.0f;

    bool zoneAt180 = (a >= (180.0f - TURN_ZONE_DEG)) && (a <= (180.0f + TURN_ZONE_DEG));
    bool zoneAt0   = (a >= (360.0f - TURN_ZONE_DEG)) || (a <= TURN_ZONE_DEG);

    return zoneAt180 || zoneAt0;
}

void MotionController::update(float rawAngleDeg, float continuousAngleDeg,
                               bool leftBrake, bool rightBrake) {
    _leftBrake = leftBrake;
    _rightBrake = rightBrake;

    // Раздел 6.2 + "поведение при первом запуске":
    // на первом цикле управления предыдущего значения ещё нет,
    // поэтому ValDelta принудительно = 0, а точка отсчёта фиксируется.
    if (!_initialized) {
        _previousContinuousAngle = continuousAngleDeg;
        _valDelta = 0.0f;
        _initialized = true;
    } else {
        _valDelta = continuousAngleDeg - _previousContinuousAngle;
        _previousContinuousAngle = continuousAngleDeg;
    }

    _inTurnZone = isInTurnZone(rawAngleDeg);

    // Раздел 8.2: порядок проверок фиксирован.
    if (leftBrake && rightBrake) {
        // 1. BOTH BRAKES
        _mode = MotionMode::CALM;
        _leftWheelDelta = 0.0f;
        _rightWheelDelta = 0.0f;

    } else if (!leftBrake && !rightBrake) {
        // 2. NO BRAKES -> NORMAL MOTION (раздел 7)
        _mode = MotionMode::NORMAL;
        float base = _valDelta * MOTOR_GEAR_RATIO / 360.0f;
        _leftWheelDelta  = base * LEFT_WHEEL_SIGN;
        _rightWheelDelta = base * RIGHT_WHEEL_SIGN;

    } else if (leftBrake && !rightBrake) {
        // 3. LEFT BRAKE ONLY -> processLeftBrake()
        if (_inTurnZone) {
            _mode = MotionMode::TURN;
            _leftWheelDelta = 0.0f;
            _rightWheelDelta = TURN_STEP * RIGHT_WHEEL_SIGN;
        } else {
            _mode = MotionMode::CALM;
            _leftWheelDelta = 0.0f;
            _rightWheelDelta = 0.0f;
        }

    } else {
        // 4. RIGHT BRAKE ONLY (rightBrake && !leftBrake) -> processRightBrake()
        if (_inTurnZone) {
            _mode = MotionMode::TURN;
            _leftWheelDelta = TURN_STEP * LEFT_WHEEL_SIGN;
            _rightWheelDelta = 0.0f;
        } else {
            _mode = MotionMode::CALM;
            _leftWheelDelta = 0.0f;
            _rightWheelDelta = 0.0f;
        }
    }
}

MotionSnapshot MotionController::getSnapshot() const {
    MotionSnapshot s;
    s.valDelta = _valDelta;
    s.leftWheelDelta = _leftWheelDelta;
    s.rightWheelDelta = _rightWheelDelta;
    s.mode = _mode;
    s.leftBrake = _leftBrake;
    s.rightBrake = _rightBrake;
    s.inTurnZone = _inTurnZone;
    return s;
}
