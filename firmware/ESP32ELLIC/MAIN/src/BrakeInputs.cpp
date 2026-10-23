#include "BrakeInputs.h"

BrakeInputs::BrakeInputs() {
    initChannel(_left, LEFT_BRAKE_PIN);
    initChannel(_right, RIGHT_BRAKE_PIN);
}

void BrakeInputs::initChannel(Channel &ch, int pin) {
    ch.pin = pin;
    ch.stable = false;
    ch.lastRaw = false;
    ch.changedMs = 0;
    ch.glitches = 0;
    ch.glitchMaxMs = 0;
}

void BrakeInputs::begin() {
    pinMode(LEFT_BRAKE_PIN, INPUT_PULLUP);
    pinMode(RIGHT_BRAKE_PIN, INPUT_PULLUP);

    uint32_t now = millis();
    Channel* channels[2] = { &_left, &_right };
    for (Channel* ch : channels) {
        ch->lastRaw = (digitalRead(ch->pin) == LOW);
        ch->stable = ch->lastRaw;
        ch->changedMs = now;
        ch->glitches = 0;
        ch->glitchMaxMs = 0;
    }
}

void BrakeInputs::updateChannel(Channel &ch, uint32_t nowMs) {
    bool raw = (digitalRead(ch.pin) == LOW);

    if (raw != ch.lastRaw) {
        // Уровень изменился. Если предыдущий уровень отличался от принятого
        // состояния и продержался меньше BRAKE_DEBOUNCE_MS - это был
        // отброшенный импульс (в любую сторону: ложное нажатие или ложное
        // отпускание).
        uint32_t width = nowMs - ch.changedMs;
        if (ch.lastRaw != ch.stable && width < BRAKE_DEBOUNCE_MS) {
            ch.glitches++;
            if (width > ch.glitchMaxMs) {
                ch.glitchMaxMs = width;
            }
        }
        ch.lastRaw = raw;
        ch.changedMs = nowMs;
        return;
    }

    if (raw != ch.stable && (nowMs - ch.changedMs) >= BRAKE_DEBOUNCE_MS) {
        ch.stable = raw;
    }
}

void BrakeInputs::update() {
    uint32_t now = millis();
    updateChannel(_left, now);
    updateChannel(_right, now);
}

BrakeSnapshot BrakeInputs::getSnapshot() const {
    BrakeSnapshot s;
    s.leftPressed = _left.stable;
    s.rightPressed = _right.stable;
    s.leftGlitches = _left.glitches;
    s.rightGlitches = _right.glitches;
    s.glitchMaxMs = (_left.glitchMaxMs > _right.glitchMaxMs) ? _left.glitchMaxMs : _right.glitchMaxMs;
    return s;
}

void BrakeInputs::resetPeriodStats() {
    _left.glitchMaxMs = 0;
    _right.glitchMaxMs = 0;
}
