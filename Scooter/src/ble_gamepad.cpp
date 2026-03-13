#include "ble_gamepad.h"
#include "config.h"
#include <Arduino.h>
#include <BleGamepad.h>

// ОПРЕДЕЛЯЕМ переменную (здесь, без extern!)
BleGamepad bleGamepad("Bluetooth gamepad", "Pico+ellic", 100);

void initBLEGamepad() {
    Serial.println("Запуск BLE Gamepad...");
    bleGamepad.begin();
    Serial.println("Геймпад запущен. Имя: Bluetooth gamepad");
}

void sendGamepadButtonPress() {
    if (!bleGamepad.isConnected()) {
        // Serial.println("Геймпад не подключен!");
        return;
    }
    
    static bool buttonPressed = false;
    static unsigned long pressStartTime = 0;
    
    if (!buttonPressed) {
        Serial.println("BLE: нажимаю BUTTON_1");
        bleGamepad.press(BUTTON_1);
        buttonPressed = true;
        pressStartTime = millis();
    }
    
    if (buttonPressed && (millis() - pressStartTime > 50)) {
        Serial.println("BLE: отпускаю BUTTON_1");
        bleGamepad.release(BUTTON_1);
        buttonPressed = false;
    }
}