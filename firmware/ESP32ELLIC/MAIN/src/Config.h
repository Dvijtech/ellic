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

// Тормоза
#define LEFT_BRAKE_PIN 32
#define RIGHT_BRAKE_PIN 33
// Тормоз нажат при digitalRead(pin) == LOW (см. раздел 4)

// CAN (ESP32 TWAI, транссивер SN65HVD230, раздел 4 / "Вопрос 3")
#define CAN_TX_PIN 16
#define CAN_RX_PIN 17
static const uint32_t CAN_BITRATE = 250000; // бит/с

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
static const uint32_t TELEMETRY_PRINT_PERIOD_MS = 100; // период printScheduled() (~10 Гц)

static const uint32_t CAN_NODE_STALE_MS = 300; // раздел 5/13: порог offline

// CAN node_id (раздел 4.3, 10.3)
static const uint8_t RIGHT_ODRIVE_NODE_ID = 1;
static const uint8_t LEFT_ODRIVE_NODE_ID  = 2;

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

// Раздел 12.3 + финальные решения "Вопрос 1/2" из спецификации:
// Vq и Procedure_Result исключены; добавлены encoderError, trajectoryDone,
// busVoltage/busCurrent (см. ODriveCAN.cpp).
struct OdriveSnapshot {
    bool online;

    int      axisState;
    uint32_t axisError;
    uint64_t motorError;       // Get Motor Error (0x03) - 64 бита у fw 0.5.6!
    uint32_t encoderError;     // Get Encoder Error (0x04)
    uint32_t controllerError;  // Get Controller Error (0x1D)
    bool     trajectoryDone;   // Heartbeat.Trajectory_Done_Flag

    float Iq;           // Get Iq (0x14), Iq_Measured
    float velEstimate;  // Get Encoder Estimates (0x09), Vel_Estimate
    float busVoltage;   // Get Bus Voltage Current (0x17)
    float busCurrent;   // Get Bus Voltage Current (0x17)

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
