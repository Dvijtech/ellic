#pragma once

#include <Arduino.h>
#include <Wire.h>

struct EncoderSnapshot {
    float rawAngle;
    float continuousAngle;
    float lastDelta;
};

class Telemetry;

class Encoder {
public:
    static constexpr uint8_t AS5600_ADDRESS = 0x36;
    static constexpr uint8_t RAW_ANGLE_MSB = 0x0C;
    static constexpr uint8_t SDA_PIN = 21;
    static constexpr uint8_t SCL_PIN = 22;
    static constexpr uint32_t I2C_CLOCK_HZ = 50000;

    explicit Encoder(Telemetry* telemetry = nullptr);

    bool begin();
    void setTelemetry(Telemetry* telemetry);
    void update();

    EncoderSnapshot getSnapshot() const;
    float getRawAngle() const;
    float getContinuousAngle() const;

private:
    Telemetry* telemetry_;
    bool initialized_;
    float rawAngle_;
    float continuousAngle_;
    float lastDelta_;
    float previousRawAngle_;

    bool readRawAngle(float& angleDeg);
    void logReadError();
};
