#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <AS5600.h>

// Обёртка над AS5600: даёт "сырой" угол 0..360 и непрерывный угол вала
class Encoder
{
public:
    bool begin(int sdaPin, int sclPin, uint32_t i2cClock = 50000)
    {
        Wire.begin(sdaPin, sclPin);
        Wire.setClock(i2cClock);

        if (!_as5600.begin())
            return false;

        if (!_as5600.magnetDetected())
            return false;

        resetBaseline();
        return true;
    }

    // Сбрасывает точку отсчёта дельты (без сброса continuousAngle)
    void resetBaseline()
    {
        _raw = readRaw();
        _lastRaw = _raw;
        _delta = 0;
    }

    // Вызывать в каждой итерации loop()
    void update()
    {
        _raw = readRaw();
        _delta = shortestDelta(_raw, _lastRaw);
        _continuous += _delta;
        _lastRaw = _raw;
    }

    float rawAngle()        const { return _raw; }
    float delta()            const { return _delta; }
    float continuousAngle() const { return _continuous; }

private:
    AS5600 _as5600;

    float _raw = 0;
    float _lastRaw = 0;
    float _delta = 0;
    float _continuous = 0;

    float readRaw()
    {
        return _as5600.readAngle() * 360.0f / 4096.0f;
    }

    static float shortestDelta(float now, float old)
    {
        float d = now - old;
        if (d > 180)  d -= 360;
        if (d < -180) d += 360;
        return d;
    }
};