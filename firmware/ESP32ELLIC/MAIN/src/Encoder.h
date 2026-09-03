#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>

class Telemetry;

struct EncoderSnapshot {
    float rawAngle;         // последний валидный raw-угол AS5600, 0..360°
    float continuousAngle;  // накопленный (безразрывный) угол вала
    float lastDelta;        // последняя учтённая дельта (по проходу loop())
};

class Encoder {
public:
    static const uint8_t AS5600_I2C_ADDRESS = 0x36;
    static const uint8_t AS5600_RAW_ANGLE_REG = 0x0C;

    Encoder();

    void setTelemetry(Telemetry* telemetry);

    void begin();
    void update(); // вызывается на каждом проходе loop(), без периода

    float getRawAngle() const;        // последний валидный raw-угол, 0..360°
    float getContinuousAngle() const; // накопленный угол

    EncoderSnapshot getSnapshot() const;

private:
    Telemetry* _telemetry;

    float _rawAngle;
    float _previousRawAngle;
    float _continuousAngle;
    float _lastDelta;
    bool  _initialized;

    bool readRawAngleFromSensor(float &outAngleDeg);
};

#endif