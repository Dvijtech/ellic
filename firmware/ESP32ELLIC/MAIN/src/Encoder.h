#pragma once
#include <Arduino.h>
#include "Config.h"

class Telemetry;

// Encoder: чтение AS5600, накопление continuousAngle (раздел 6.1 спецификации).
// update() вызывается на каждом проходе loop(), но сам читает датчик не чаще
// одного раза в ENCODER_PERIOD_MS.
class Encoder {
public:
    Encoder();

    // telemetry может быть nullptr
    void begin(Telemetry* telemetry);

    // Вызывать в каждом проходе loop().
    void update();

    EncoderSnapshot getSnapshot() const;

    // Сбрасывает статистику за период (readMaxUs). Вызывает Telemetry после
    // выдачи пакета.
    void resetPeriodStats();

private:
    bool readRawAngleDeg(float &outDeg);

    Telemetry* _telemetry;

    bool  _initialized;
    float _previousRawAngle;  // последнее валидное сырое значение, град.
    float _continuousAngle;   // накопленный угол, град.
    float _lastDelta;         // последняя посчитанная дельта (уровень энкодера)

    uint32_t _readErrors;     // накопительный счётчик неудачных чтений

    bool     _hasRead;        // было ли хотя бы одно обращение к датчику
    uint32_t _lastReadMs;     // момент последнего обращения к датчику
    uint32_t _readMaxUs;      // максимум длительности чтения за период
};
