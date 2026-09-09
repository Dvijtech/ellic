#pragma once

#include <Arduino.h>
#include "Encoder.h"
#include "ODriveCAN.h"

struct MotionSnapshot {
    float valDelta;
    float leftWheelDelta;
    float rightWheelDelta;
    enum class Mode { NORMAL, CALM, TURN } mode;
    bool leftBrake;
    bool rightBrake;
    bool inTurnZone;
};

class Telemetry;

class MotionController {
public:
    static constexpr uint32_t CONTROL_PERIOD_MS = 300;
    static constexpr float MOTOR_GEAR_RATIO = 4.4f;
    static constexpr float TURN_ZONE_DEG = 10.0f;
    static constexpr float TURN_STEP = 0.03f;
    static constexpr float LEFT_WHEEL_SIGN = 1.0f;
    static constexpr float RIGHT_WHEEL_SIGN = -1.0f;
    static constexpr uint8_t LEFT_BRAKE_PIN = 32;
    static constexpr uint8_t RIGHT_BRAKE_PIN = 33;

    MotionController(Encoder& encoder, ODriveCAN& odrive, Telemetry* telemetry = nullptr);

    void begin();
    void update();
    MotionSnapshot getSnapshot() const;

private:
    Encoder& encoder_;
    ODriveCAN& odrive_;
    Telemetry* telemetry_;

    uint32_t lastControlMs_;
    bool controlInitialized_;
    float previousControlAngle_;
    MotionSnapshot snapshot_;

    bool isTurnZone(float rawAngle) const;
    void setCalm();
    void setNormal(float valDelta);
    void processLeftBrake(float rawAngle);
    void processRightBrake(float rawAngle);
};
