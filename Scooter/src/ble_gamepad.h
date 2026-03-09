#ifndef BLE_GAMEPAD_H
#define BLE_GAMEPAD_H

#include <BleGamepad.h>

// Объявляем внешнюю переменную, чтобы её можно было использовать в других файлах
extern BleGamepad bleGamepad;

void initBLEGamepad();
void sendGamepadButtonPress();

#endif