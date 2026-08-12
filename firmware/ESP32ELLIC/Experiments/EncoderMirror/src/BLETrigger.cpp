#include "BLETrigger.h"
#include <Arduino.h>
#include <BleGamepad.h>

BleGamepad bleGamepad("ESP32 Steering Wheel", "DIY", 100);

// Пин нажатия на джойстик (Кнопка Y)
constexpr int JOY_BTN_PIN = 27;

// Аналоговые оси джойстика
constexpr int JOY_X_PIN = 34; // VRX -> GPIO 34 (Input Only)
constexpr int JOY_Y_PIN = 35; // VRY -> GPIO 35 (Input Only)

// Переменные антидребезга для кнопки
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
constexpr unsigned long DEBOUNCE_DELAY = 25;

void BLETrigger_begin()
{
    pinMode(JOY_BTN_PIN, INPUT_PULLUP);
    analogReadResolution(12); // 0..4095
    bleGamepad.begin();
}

void BLETrigger_update()
{
    if (!bleGamepad.isConnected())
        return;

    // --- 1. Обработка кнопки Y (нажатие SW на HW-504) ---
    bool rawButtonState = digitalRead(JOY_BTN_PIN);

    if (rawButtonState != lastButtonState && (millis() - lastDebounceTime) > DEBOUNCE_DELAY)
    {
        lastDebounceTime = millis();
        lastButtonState = rawButtonState;

        // При нажатии 3.3V падает до 0V (LOW)
        if (rawButtonState == LOW)
        {
            bleGamepad.press(BUTTON_4); // Кнопка Y (Gamepad Face Button Top)
        }
        else
        {
            bleGamepad.release(BUTTON_4);
        }
    }

    // --- 2. Обработка осей джойстика HW-504 ---
    int xVal = analogRead(JOY_X_PIN);
    int yVal = analogRead(JOY_Y_PIN);

    // Центрирование от -32767 до 32767
    int16_t mappedX = map(xVal, 0, 4095, -32767, 32767);
   
    // Инверсия Y (4095..0 вместо 0..4095) для корректного инпута "вверх/вниз"
    int16_t mappedY = map(yVal, 0, 4095, 32767, -32767);

    bleGamepad.setLeftThumb(mappedX, mappedY);
}