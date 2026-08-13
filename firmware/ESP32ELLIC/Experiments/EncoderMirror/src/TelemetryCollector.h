#pragma once
#include "ODriveUART.h"

struct TelemetrySample
{
    uint32_t tMs = 0;
    float rawAngle = 0, targetTurns = 0;
    float leftCurrent = 0, leftVoltage = 0, leftVelocity = 0;
    float rightCurrent = 0, rightVoltage = 0, rightVelocity = 0;
    bool leftBrake = false, rightBrake = false, turnZone = false, turning = false;
};

// Асинхронно опрашивает ток/напряжение/скорость обоих приводов по кругу.
// Живость привода = успех/таймаут этих же запросов (отдельный пинг не нужен).
class TelemetryCollector
{
public:
    TelemetryCollector(ODriveUART &left, ODriveUART &right, uint32_t periodMs = 150);

    void begin();
    void update(); // вызывать каждый loop()

    // Пишется снаружи перед update() — данные, не относящиеся к ODrive
    void setContext(float rawAngle, float targetTurns,
                     bool leftBrake, bool rightBrake, bool turnZone, bool turning);

    bool sampleReady(); // true один раз за цикл
    const TelemetrySample &sample() const { return _sample; }

private:
    enum class Step { LeftCurrent, LeftVoltage, LeftVelocity,
                       RightCurrent, RightVoltage, RightVelocity, Done };

    ODriveUART &_left, &_right;
    uint32_t _periodMs;
    unsigned long _cycleStart = 0;
    Step _step = Step::Done;
    TelemetrySample _sample;
    bool _ready = false;

    void startStep(Step step);
};