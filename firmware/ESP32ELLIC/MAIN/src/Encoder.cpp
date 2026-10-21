#include "Encoder.h"
#include "Telemetry.h"
#include <Wire.h>

Encoder::Encoder()
    : _telemetry(nullptr),
      _initialized(false),
      _previousRawAngle(0.0f),
      _continuousAngle(0.0f),
      _lastDelta(0.0f),
      _readErrors(0),
      _hasRead(false),
      _lastReadMs(0),
      _readMaxUs(0) {}

void Encoder::begin(Telemetry* telemetry) {
    _telemetry = telemetry;
    _initialized = false;
    _previousRawAngle = 0.0f;
    _continuousAngle = 0.0f;
    _lastDelta = 0.0f;
    _readErrors = 0;
    _hasRead = false;
    _lastReadMs = 0;
    _readMaxUs = 0;
}

// Чтение 12-битного RAW ANGLE регистра AS5600 (0x0C/0x0D) -> градусы 0..360.
// Возвращает false, если Wire.requestFrom() вернул менее 2 байт (раздел 6.3).
bool Encoder::readRawAngleDeg(float &outDeg) {
    Wire.beginTransmission(AS5600_I2C_ADDR);
    Wire.write(AS5600_RAW_ANGLE_REG);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }

    uint8_t received = Wire.requestFrom((int)AS5600_I2C_ADDR, (int)2);
    if (received < 2) {
        // Недостаточно байт - ошибка чтения (раздел 6.3)
        return false;
    }

    uint8_t hi = Wire.read();
    uint8_t lo = Wire.read();
    uint16_t raw12 = ((uint16_t)(hi & 0x0F) << 8) | lo;

    outDeg = (raw12 * 360.0f) / 4096.0f;
    return true;
}

void Encoder::update() {
    // Раздел 6.1: датчик опрашивается раз в ENCODER_PERIOD_MS, а не на каждом
    // проходе loop() - меньше трафика на I2C и больше времени на приём CAN.
    uint32_t nowMs = millis();
    if (_hasRead && (nowMs - _lastReadMs) < ENCODER_PERIOD_MS) {
        return;
    }
    _hasRead = true;
    _lastReadMs = nowMs;

    float raw;
    uint32_t t0 = micros();
    bool ok = readRawAngleDeg(raw);
    uint32_t dur = micros() - t0;
    if (dur > _readMaxUs) {
        _readMaxUs = dur;
    }

    if (!ok) {
        // Раздел 6.3: continuousAngle и previousRawAngle НЕ обновляются,
        // снапшот продолжает отдавать последнее валидное значение.
        _readErrors++;
        return;
    }

    if (!_initialized) {
        // Первый успешный запуск: фиксируем точку отсчёта, дельту не считаем.
        _previousRawAngle = raw;
        _continuousAngle = 0.0f;
        _lastDelta = 0.0f;
        _initialized = true;
        return;
    }

    float delta = raw - _previousRawAngle;
    if (delta > 180.0f) {
        delta -= 360.0f;
    } else if (delta < -180.0f) {
        delta += 360.0f;
    }

    _continuousAngle += delta;
    _lastDelta = delta;
    _previousRawAngle = raw;
}

EncoderSnapshot Encoder::getSnapshot() const {
    EncoderSnapshot s;
    s.rawAngle = _previousRawAngle;
    s.continuousAngle = _continuousAngle;
    s.lastDelta = _lastDelta;
    s.readErrors = _readErrors;
    s.readMaxUs = _readMaxUs;
    return s;
}

void Encoder::resetPeriodStats() {
    _readMaxUs = 0;
}
