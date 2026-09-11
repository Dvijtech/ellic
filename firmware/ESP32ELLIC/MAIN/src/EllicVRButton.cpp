#include "EllicVRButton.h"

EllicVRButton::EllicVRButton(const char* deviceName, int pin) 
    : bleGamepad(deviceName, "ELLIC", 100), buttonPin(pin), lastButtonState(HIGH) {
}

void EllicVRButton::begin() {
    pinMode(buttonPin, INPUT_PULLUP);
    bleGamepad.begin();
}

void EllicVRButton::update() {
    if (bleGamepad.isConnected()) {
        int currentButtonState = digitalRead(buttonPin);

        // Нажатие кнопки (HIGH -> LOW)
        if (lastButtonState == HIGH && currentButtonState == LOW) {
            bleGamepad.press(BUTTON_1); // Кнопка A / South Button
            delay(50); // Дебаунс
        }

        // Отпускание кнопки (LOW -> HIGH)
        if (lastButtonState == LOW && currentButtonState == HIGH) {
            bleGamepad.release(BUTTON_1);
            delay(50);
        }

        lastButtonState = currentButtonState;
    }
}