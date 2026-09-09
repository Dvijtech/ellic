#pragma once
#include <Arduino.h>
#include "Config.h"

class Telemetry;

// Encoder: чтение AS5600, накопление continuousAngle.
// Вызывается на каждом проходе loop(), без привязки к CONTROL_PERIOD_MS
// (раздел 1 модулей / раздел 6.1 спецификации).
class Encoder {
public:
    Encoder();

    // telemetry может быть nullptr, тогда ошибки чтения просто не логируются
    void begin(Telemetry* telemetry);

    // Вызывать в каждом проходе loop().
    void update();

    EncoderSnapshot getSnapshot() const;

private:
    bool readRawAngleDeg(float &outDeg);

    Telemetry* _telemetry;

    bool  _initialized;
    float _previousRawAngle;  // последнее валидное сырое значение, град.
    float _continuousAngle;   // накопленный угол, град.
    float _lastDelta;         // последняя посчитанная дельта (уровень энкодера)
};
