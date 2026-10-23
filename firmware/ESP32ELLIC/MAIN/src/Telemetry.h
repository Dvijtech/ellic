#pragma once
#include <Arduino.h>
#include "Config.h"

class Encoder;
class MotionController;
class ODriveCAN;
class BleTelemetry;
class FireButton;
class BrakeInputs;

// Telemetry: единственный модуль, отвечающий за вывод телеметрии (раздел 13).
// Раз в TELEMETRY_PERIOD_MS собирает снапшоты и печатает ОДНИМ буфером пакет из
// записей M, L, R, E, D, C (Serial + BLE). Один буфер вместо шести отдельных
// вызовов - BLE-стек ограничен числом notify() в секунду, а не байтами.
class Telemetry {
public:
    Telemetry(Encoder* encoder, MotionController* motion, ODriveCAN* odriveCAN);

    void begin();

    // Опционально подключить BLE-канал (по умолчанию только Serial).
    void setBleTelemetry(BleTelemetry* ble);

    // Подключить кнопку стрельбы (поля fire / fire_count в конце записи M).
    void setFireButton(FireButton* fire);

    // Подключить входы тормозов (счётчики отброшенных импульсов в E и D).
    void setBrakeInputs(BrakeInputs* brakes);

    // Сразу, вне расписания, выдаёт одну строку M с текущим состоянием кнопки.
    // Вызывать при изменении состояния кнопки (раздел 13.2 спецификации).
    void sendFireEvent();

    // Вызывать в каждом проходе loop().
    void update();

    // Вызывать в начале каждого прохода loop(): интервал (мкс) между началами
    // двух соседних проходов. Максимум за период уходит в запись D.
    void noteLoopGap(uint32_t gapUs);

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
    FireButton* _fire;
    BrakeInputs* _brakes;

    uint32_t _lastMs;
    uint32_t _prevAs5600;
    uint32_t _prevAsRecov;
    uint32_t _prevBrakeL;
    uint32_t _prevBrakeR;
    WheelCounters _prevLeft;
    WheelCounters _prevRight;

    // Для записей C и D
    uint32_t _prevCmdLeft[ODRIVE_CMD_SLOTS];
    uint32_t _prevCmdRight[ODRIVE_CMD_SLOTS];
    CanBusSnapshot _prevBus;

    uint32_t _loopGapMaxUs;   // максимум интервала между проходами loop() за период
    uint32_t _lastSendUs;     // длительность предыдущей выдачи пакета (печать + постановка в BLE)

    int      _resetReason;    // esp_reset_reason() этого запуска
    uint32_t _bootCount;      // число запусков (хранится в RTC-памяти, переживает перезагрузку)
    uint32_t _prevUptimeS;    // uptime предыдущего запуска на момент его последней выдачи
};
