#include "ble_keyboard.h"
#include <Arduino.h>

// Создаём экземпляр BleKeyboard с именем устройства
BleKeyboard bleKeyboard("ESP32 Joystick");

void initBLEKeyboard() {
    bleKeyboard.begin();
}

void sendKeyPress(char key) {
    if (bleKeyboard.isConnected()) {
        bleKeyboard.press(key);
        delay(20);     // небольшая задержка для регистрации нажатия
        bleKeyboard.releaseAll();
    }
}