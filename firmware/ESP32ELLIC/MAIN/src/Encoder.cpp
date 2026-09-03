#include "Encoder.h"
#include "Telemetry.h"
#include <Wire.h>

Encoder::Encoder()
: _telemetry(nullptr),
  _rawAngle(0.0f),
  _previousRawAngle(0.0f),
  _continuousAngle(0.0f),
  _lastDelta(0.0f),
  _initialized(false)
{
}

void Encoder::setTelemetry(Telemetry* telemetry) {
    _telemetry = telemetry;
}

void Encoder::begin() {
    // Wire.begin()/Wire.setClock() выполняются один раз в main.cpp::setup(),
    // так как I2C-шина общая для системы; Encoder её не инициализирует.
}

bool Encoder::readRawAngleFromSensor(float &outAngleDeg) {
    Wire.beginTransmission(AS5600_I2C_ADDRESS);
    Wire.write(AS5600_RAW_ANGLE_REG);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }

    uint8_t got = Wire.requestFrom((int)AS5600_I2C_ADDRESS, 2);
    if (got < 2 || Wire.available() < 2) {
        return false;
    }

    uint16_t hi = Wire.read();
    uint16_t lo = Wire.read();
    uint16_t raw = ((hi << 8) | lo) & 0x0FFF; // 12 бит

    outAngleDeg = (raw * 360.0f) / 4096.0f;
    return true;
}

void Encoder::update() {
    float newRaw;
    bool ok = readRawAngleFromSensor(newRaw);

    if (!ok) {
        // 6.3: continuousAngle и previousRawAngle НЕ обновляются
        if (_telemetry) {
            _telemetry->log(LogLevel::WARNING, "Encoder", "AS5600 read error (I2C < 2 bytes)");
        }
        return;
    }

    if (!_initialized) {
        // Первый валидный отсчёт: только инициализация, без вычисления дельты
        _previousRawAngle = newRaw;
        _rawAngle = newRaw;
        _continuousAngle = 0.0f;
        _lastDelta = 0.0f;
        _initialized = true;
        return;
    }

    float delta = newRaw - _previousRawAngle;
    if (delta > 180.0f)  delta -= 360.0f;
    if (delta < -180.0f) delta += 360.0f;

    _continuousAngle += delta;
    _previousRawAngle = newRaw;
    _rawAngle = newRaw;
    _lastDelta = delta;
}

float Encoder::getRawAngle() const { return _rawAngle; }
float Encoder::getContinuousAngle() const { return _continuousAngle; }

EncoderSnapshot Encoder::getSnapshot() const {
    EncoderSnapshot s;
    s.rawAngle = _rawAngle;
    s.continuousAngle = _continuousAngle;
    s.lastDelta = _lastDelta;
    return s;
}