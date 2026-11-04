#include "BrakeInputs.h"

BrakeInputs::BrakeInputs() {}

void BrakeInputs::begin() {
    pinMode(LEFT_BRAKE_PIN, INPUT_PULLUP);
    pinMode(RIGHT_BRAKE_PIN, INPUT_PULLUP);

    uint32_t nowUs = micros();
    _left.begin(digitalRead(LEFT_BRAKE_PIN) == LOW, nowUs, BRAKE_DEBOUNCE_MS);
    _right.begin(digitalRead(RIGHT_BRAKE_PIN) == LOW, nowUs, BRAKE_DEBOUNCE_MS);
}

void BrakeInputs::update() {
    uint32_t nowUs = micros();
    _left.update(digitalRead(LEFT_BRAKE_PIN) == LOW, nowUs);
    _right.update(digitalRead(RIGHT_BRAKE_PIN) == LOW, nowUs);
}

BrakeSnapshot BrakeInputs::getSnapshot() const {
    BrakeSnapshot s;
    s.leftPressed = _left.stable();
    s.rightPressed = _right.stable();
    s.leftGlitches = _left.glitches();
    s.rightGlitches = _right.glitches();
    s.glitchMaxMs = (_left.glitchMaxMs() > _right.glitchMaxMs()) ? _left.glitchMaxMs() : _right.glitchMaxMs();
    return s;
}

void BrakeInputs::resetPeriodStats() {
    _left.resetPeriodStats();
    _right.resetPeriodStats();
}
