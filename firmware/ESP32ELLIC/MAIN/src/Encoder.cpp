#include "Encoder.h"
#include "Telemetry.h"

Encoder::Encoder(Telemetry* telemetry)
    : telemetry_(telemetry),
      initialized_(false),
      rawAngle_(0.0f),
      continuousAngle_(0.0f),
      lastDelta_(0.0f),
      previousRawAngle_(0.0f) {}

void Encoder::setTelemetry(Telemetry* telemetry) { telemetry_ = telemetry; }

bool Encoder::begin() {
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(I2C_CLOCK_HZ);

    float angle = 0.0f;
    if (!readRawAngle(angle)) {
        initialized_ = false;
        return false;
    }

    rawAngle_ = angle;
    previousRawAngle_ = angle;
    continuousAngle_ = angle;
    lastDelta_ = 0.0f;
    initialized_ = true;
    return true;
}

void Encoder::update() {
    float currentRawAngle = 0.0f;
    if (!readRawAngle(currentRawAngle)) {
        logReadError();
        return;
    }

    rawAngle_ = currentRawAngle;

    if (!initialized_) {
        previousRawAngle_ = currentRawAngle;
        continuousAngle_ = currentRawAngle;
        lastDelta_ = 0.0f;
        initialized_ = true;
        return;
    }

    float delta = currentRawAngle - previousRawAngle_;

    if (delta > 180.0f) {
        delta -= 360.0f;
    } else if (delta < -180.0f) {
        delta += 360.0f;
    }

    continuousAngle_ += delta;
    lastDelta_ = delta;
    previousRawAngle_ = currentRawAngle;
}

EncoderSnapshot Encoder::getSnapshot() const {
    return {rawAngle_, continuousAngle_, lastDelta_};
}

float Encoder::getRawAngle() const {
    return rawAngle_;
}

float Encoder::getContinuousAngle() const {
    return continuousAngle_;
}

bool Encoder::readRawAngle(float& angleDeg) {
    Wire.beginTransmission(AS5600_ADDRESS);
    Wire.write(RAW_ANGLE_MSB);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }

    uint8_t received = Wire.requestFrom(AS5600_ADDRESS, static_cast<uint8_t>(2));
    if (received < 2) {
        while (Wire.available()) {
            Wire.read();
        }
        return false;
    }

    const uint16_t raw = (static_cast<uint16_t>(Wire.read()) << 8) |
                         static_cast<uint16_t>(Wire.read());
    const uint16_t raw12 = raw & 0x0FFF;
    angleDeg = (static_cast<float>(raw12) * 360.0f) / 4096.0f;
    return true;
}

void Encoder::logReadError() {
    if (telemetry_ != nullptr) {
        telemetry_->log(LogLevel::WARNING, "Encoder", "AS5600 read error");
    }
}
