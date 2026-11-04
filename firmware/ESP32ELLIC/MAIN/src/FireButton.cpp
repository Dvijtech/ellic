#include "FireButton.h"

FireButton::FireButton() : _pressCount(0) {}

void FireButton::begin() {
    pinMode(FIRE_BUTTON_PIN, INPUT_PULLUP);
    _filter.begin(digitalRead(FIRE_BUTTON_PIN) == LOW, micros(), FIRE_DEBOUNCE_MS);
    _pressCount = 0;
}

bool FireButton::update() {
    bool changed = _filter.update(digitalRead(FIRE_BUTTON_PIN) == LOW, micros());
    if (changed && _filter.stable()) {
        _pressCount++;
    }
    return changed;
}

FireSnapshot FireButton::getSnapshot() const {
    FireSnapshot s;
    s.pressed = _filter.stable();
    s.pressCount = _pressCount;
    return s;
}
