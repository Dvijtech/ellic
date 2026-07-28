#include "BLETrigger.h"

#include <Arduino.h>
#include <BleGamepad.h>

BleGamepad bleGamepad("VR_Trigger", "ESP32", 100);

constexpr int BUTTON_PIN = 27;

bool lastState = HIGH;

void BLETrigger_begin()
{
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    bleGamepad.begin();
}

void BLETrigger_update()
{
    if (!bleGamepad.isConnected())
        return;

    bool state = digitalRead(BUTTON_PIN);

    // кнопка нажата
    if (state == LOW && lastState == HIGH)
    {
        bleGamepad.press(BUTTON_1);
    }

    // кнопка отпущена
    if (state == HIGH && lastState == LOW)
    {
        bleGamepad.release(BUTTON_1);
    }

    lastState = state;
}