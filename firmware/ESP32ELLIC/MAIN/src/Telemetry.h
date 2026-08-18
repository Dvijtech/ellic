#pragma once

#include <Arduino.h>
#include "ODriveUART.h"

// ============================================================
// TELEMETRY SAMPLE
// ============================================================

struct TelemetrySample
{
    uint32_t tMs = 0;

    // Энкодер / управление
    float rawAngle = 0;
    float targetTurns = 0;

    // LEFT ODrive
    float leftCurrent = 0;
    float leftVoltage = 0;
    float leftVelocity = 0;

    // RIGHT ODrive
    float rightCurrent = 0;
    float rightVoltage = 0;
    float rightVelocity = 0;

    // Состояние механизма
    bool leftBrake = false;
    bool rightBrake = false;
    bool turnZone = false;
    bool turning = false;

    // Отладочная информация
    float leftHoldOffset = 0;
    float rightHoldOffset = 0;
};

// ============================================================
// TELEMETRY
// ============================================================
//
// Отвечает за:
//
// 1. Асинхронный опрос ODrive.
// 2. Сбор состояния системы.
// 3. Формирование TelemetrySample.
// 4. Периодический Serial debug.
//
// ============================================================

class Telemetry
{
public:

    Telemetry(
        ODriveUART &left,
        ODriveUART &right,
        uint32_t periodMs = 150,
        uint32_t printPeriodMs = 100);

    void begin();

    // Вызывать каждый loop()
    void update();

    // Передаёт текущий контекст системы
    void setContext(
        float rawAngle,
        float targetTurns,
        bool leftBrake,
        bool rightBrake,
        bool turnZone,
        bool turning,
        float leftHoldOffset,
        float rightHoldOffset);

    // true один раз после завершения полного цикла
    bool sampleReady();

    const TelemetrySample &sample() const
    {
        return _sample;
    }

    // Логи состояния ODrive
    static void logOdriveReinit(const char *name);
    static void logOdriveReady(const char *name);
    static void logOdrivePowerRestored(const char *name);

private:

    enum class Step
    {
        LeftCurrent,
        LeftVoltage,
        LeftVelocity,

        RightCurrent,
        RightVoltage,
        RightVelocity,

        Done
    };

    ODriveUART &_left;
    ODriveUART &_right;

    uint32_t _periodMs;
    uint32_t _printPeriodMs;

    unsigned long _cycleStart = 0;
    unsigned long _lastPrint = 0;

    Step _step = Step::Done;

    TelemetrySample _sample;

    bool _ready = false;

    void startStep(Step step);

    void printDebug();
};