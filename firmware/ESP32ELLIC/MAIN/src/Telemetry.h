#pragma once
#include <Arduino.h>
#include "Config.h"

class Encoder;
class MotionController;
class ODriveCAN;
class BleTelemetry;

// Telemetry: единственный модуль, отвечающий за вывод телеметрии.
// Протокол v2 (ELLIC_telemetry_TZ.md): раз в TELEMETRY_PERIOD_MS собирает
// снапшоты и сразу печатает пакет из записей M, L, R, E (Serial + BLE).
class Telemetry {
public:
    Telemetry(Encoder* encoder, MotionController* motion, ODriveCAN* odriveCAN);

    void begin();

    // Опционально подключить BLE-канал (по умолчанию только Serial).
    void setBleTelemetry(BleTelemetry* ble);

    // Вызывать в каждом проходе loop().
    void update();

private:
    // Значения накопительных счётчиков колеса на момент прошлой выдачи.
    // Нужны, чтобы выдавать события за период, а не накопленный итог.
    struct WheelCounters {
        uint32_t tx, rx, txFail, stale, offline, axisErr, motErr, encErr, ctrlErr;
    };
    static WheelCounters countersOf(const OdriveSnapshot &s);

    void sendPacket(uint32_t ts);
    void emit(const char* text) const;

    Encoder* _encoder;
    MotionController* _motion;
    ODriveCAN* _odriveCAN;
    BleTelemetry* _ble;

    uint32_t _lastMs;
    uint32_t _prevAs5600;
    WheelCounters _prevLeft;
    WheelCounters _prevRight;
};