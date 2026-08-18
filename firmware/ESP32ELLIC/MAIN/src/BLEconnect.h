#pragma once

#include <Arduino.h>
#include "Telemetry.h"

// Инициализация BLE Gamepad + BLE Telemetry
void BLEconnect_begin();

// Обработка джойстика и кнопки
void BLEconnect_update();

// Передача полного telemetry-пакета по BLE
void BLEconnect_sendTelemetry(const TelemetrySample &sample);