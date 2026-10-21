#include "FireButton.h"

FireButton::FireButton()
    : _pressed(false),
      _lastRaw(false),
      _rawChangedMs(0),
      _pressCount(0) {}

void FireButton::begin() {
    pinMode(FIRE_BUTTON_PIN, INPUT_PULLUP);
    _lastRaw = (digitalRead(FIRE_BUTTON_PIN) == LOW);
    _pressed = _lastRaw;
    _rawChangedMs = millis();
    _pressCount = 0;
}

bool FireButton::update() {
    bool raw = (digitalRead(FIRE_BUTTON_PIN) == LOW);
    uint32_t now = millis();

    if (raw != _lastRaw) {
        // Сигнал изменился - отсчёт устойчивости начинается заново.
        _lastRaw = raw;
        _rawChangedMs = now;
        return false;
    }

    if (raw != _pressed && (now - _rawChangedMs) >= FIRE_DEBOUNCE_MS) {
        _pressed = raw;
        if (_pressed) {
            _pressCount++;
        }
        return true;
    }
    return false;
}

FireSnapshot FireButton::getSnapshot() const {
    FireSnapshot s;
    s.pressed = _pressed;
    s.pressCount = _pressCount;
    return s;
}
