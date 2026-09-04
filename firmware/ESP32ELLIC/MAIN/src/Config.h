#pragma once
#include <Arduino.h>

// =======================================================================
// Аппаратные подключения (раздел 4 спецификации)
// =======================================================================

// AS5600 (I2C)
#define AS5600_SDA_PIN 21
#define AS5600_SCL_PIN 22
#define AS5600_I2C_CLOCK_HZ 50000

// AS5600 регистр RAW ANGLE (12 бит, 0x0C/0x0D)
#define AS5600_I2C_ADDR 0x36
#define AS5600_RAW_ANGLE_REG 0x0C

// LEFT ODrive (UART)
#define ODRIVE_LEFT_RX_PIN 4
#define ODRIVE_LEFT_TX_PIN 5

// RIGHT ODrive (UART)
#define ODRIVE_RIGHT_RX_PIN 26
#define ODRIVE_RIGHT_TX_PIN 25

#define ODRIVE_UART_BAUD 115200

// Тормоза
#define LEFT_BRAKE_PIN 32
#define RIGHT_BRAKE_PIN 33
// Тормоз нажат при digitalRead(pin) == LOW (см. раздел 4)

// =======================================================================
// Константы (раздел 5 спецификации)
// =======================================================================

static const uint32_t CONTROL_PERIOD_MS      = 300;   // период MotionController.update()
static const float    MOTOR_GEAR_RATIO       = 4.4f;  // передаточное число редуктора
static const float    TURN_ZONE_DEG          = 10.0f; // ширина зоны поворота
static const float    TURN_STEP              = 0.03f; // шаг противоположного колеса при повороте (оборотов)
static const float    LEFT_WHEEL_SIGN        = 1.0f;
static const float    RIGHT_WHEEL_SIGN       = -1.0f;

static const uint32_t TELEMETRY_PERIOD_MS       = 150; // период collect()
static const uint32_t TELEMETRY_PRINT_PERIOD_MS = 1000; // период printScheduled()

static const uint32_t RESPONSE_TIMEOUT_MS   = 50;   // таймаут ответа UART ODrive
static const int      ALIVE_FAIL_THRESHOLD  = 5;    // подряд неудач -> offline
static const uint32_t DIAG_PERIOD_MS        = 1000; // мин. интервал фоновой диагностики

// =======================================================================
// Общие перечисления и структуры-снапшоты (раздел 12.3)
// =======================================================================

enum class LogLevel { INFO, WARNING, ERROR, CRITICAL };

enum class MotionMode { NORMAL, CALM, TURN };

struct EncoderSnapshot {
    float rawAngle;        // последнее валидное значение AS5600, 0..360
    float continuousAngle; // накопленный (безразрывный) угол
    float lastDelta;       // последняя дельта, посчитанная в Encoder::update()
};

struct MotionSnapshot {
    float valDelta;
    float leftWheelDelta;
    float rightWheelDelta;
    MotionMode mode;
    bool leftBrake;
    bool rightBrake;
    bool inTurnZone;
};

struct OdriveSnapshot {
    bool online;
    int axisState;
    int axisError;
    int motorError;
    int controllerError;
    float Iq;
    float Vq;
    float velEstimate;
    uint32_t txCount;
    uint32_t rxCount;
    uint32_t rxFailCount;
    uint32_t diagnosticsTimestampMs;
};

struct TelemetrySample {
    EncoderSnapshot encoder;
    MotionSnapshot  motion;
    OdriveSnapshot  left;
    OdriveSnapshot  right;
};
