#ifndef BLE_GAMEPAD_H
#define BLE_GAMEPAD_H

#include <BleGamepad.h>

// Объявляем внешнюю переменную
extern BleGamepad bleGamepad;

// Объявляем функции
void initBLEGamepad();
void sendGamepadButtonPress();

#endif