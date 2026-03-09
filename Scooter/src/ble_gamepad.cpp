#include "ble_gamepad.h"
#include "config.h"
#include <Arduino.h>

// Создаём экземпляр геймпада
// Параметры: имя производителя, имя устройства, уровень заряда батареи (0-100)
BleGamepad bleGamepad("ESP32 Project", "PICO Controller", 100);

void initBLEGamepad() {
    Serial.println("Запуск BLE Gamepad...");
    bleGamepad.begin();
    Serial.println("Геймпад запущен. Имя: PICO Controller");
}

void sendGamepadButtonPress() {
    if (bleGamepad.isConnected()) {
        Serial.println("Отправляем нажатие кнопки BUTTON_1");
        bleGamepad.press(BUTTON_1);  // Нажимаем первую кнопку
        delay(10);                    // Маленькая пауза для регистрации нажатия
        bleGamepad.release(BUTTON_1);      // Отпускаем все кнопки
        Serial.println("Кнопка отпущена");
    } else {
        Serial.println("Геймпад не подключен к PICO");
    }
}