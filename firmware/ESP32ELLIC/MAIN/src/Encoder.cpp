#include "Encoder.h"
#include "Telemetry.h"
#include <Wire.h>

Encoder::Encoder()
    : _telemetry(nullptr),
      _initialized(false),
      _previousRawAngle(0.0f),
      _continuousAngle(0.0f),
      _lastDelta(0.0f) {}

void Encoder::begin(Telemetry* telemetry) {
    _telemetry = telemetry;
    _initialized = false;
    _previousRawAngle = 0.0f;
    _continuousAngle = 0.0f;
    _lastDelta = 0.0f;
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
    float raw;
    if (!readRawAngleDeg(raw)) {
        // Раздел 6.3: continuousAngle и previousRawAngle НЕ обновляются,
        // функция(снапшот) продолжает отдавать последнее валидное значение.
        if (_telemetry != nullptr) {
            _telemetry->log(LogLevel::WARNING, "Encoder", "AS5600 read error (<2 bytes)");
        }
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
    return s;
}
