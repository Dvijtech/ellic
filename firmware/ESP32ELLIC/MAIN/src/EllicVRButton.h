#ifndef ELLIC_VR_BUTTON_H
#define ELLIC_VR_BUTTON_H

#include <Arduino.h>
#include <BleGamepad.h>

class EllicVRButton {
private:
    BleGamepad bleGamepad;
    int buttonPin;
    int lastButtonState;

public:
    // Конструктор
    EllicVRButton(const char* deviceName = "ELLIC VR Gamepad", int pin = 18);

    // Инициализация пина и BLE
    void begin();

    // Опрос состояния кнопки (вызывается в loop)
    void update();
};

#endif // ELLIC_VR_BUTTON_H