#include <Arduino.h>

#include "Encoder.h"
#include "ODriveUART.h"
#include "PhaseDetector.h"
#include "MotionController.h"
#include "Telemetry.h"
#include "BLEconnect.h"

// ============================================================
// PINS
// ============================================================

constexpr int SDA_PIN = 21;
constexpr int SCL_PIN = 22;

constexpr int LEFT_RX = 4; // желтый
constexpr int LEFT_TX = 5; // зеленый 

constexpr int RIGHT_RX = 26;
constexpr int RIGHT_TX = 25;

constexpr int LEFT_BRAKE_PIN = 32;
constexpr int RIGHT_BRAKE_PIN = 33;

// ============================================================
// MOTION PARAMETERS
// ============================================================

constexpr float GEAR_RATIO = 4.4f;
constexpr float TURN_STEP = 0.03f;

constexpr uint32_t CONTROL_PERIOD = 50;

constexpr uint32_t TELEMETRY_PERIOD = 150;

constexpr uint32_t PRINT_PERIOD = 100;

constexpr uint32_t ODRIVE_BOOT_DELAY = 1500;

// ============================================================
// OBJECTS
// ============================================================

Encoder valEncoder;

ODriveUART leftDrive(
    Serial1,
    "LEFT");

ODriveUART rightDrive(
    Serial2,
    "RIGHT");

PhaseDetector phase;

MotionController motion(
    valEncoder,
    leftDrive,
    rightDrive,
    phase,
    GEAR_RATIO,
    TURN_STEP,
    CONTROL_PERIOD);

Telemetry telemetry(
    leftDrive,
    rightDrive,
    TELEMETRY_PERIOD,
    PRINT_PERIOD);

// ============================================================
// ODRIVE ALIVE STATE
// ============================================================

bool leftAlivePrev = false;
bool rightAlivePrev = false;

// ============================================================
// ODRIVE RECOVERY
// ============================================================

void handleAliveEdges()
{
    // --------------------------------------------------------
    // LEFT
    // --------------------------------------------------------

    bool leftNow = leftDrive.isAlive();

    if (leftNow && !leftAlivePrev)
    {
        Telemetry::logOdrivePowerRestored("LEFT");

        Telemetry::logOdriveReinit("LEFT");

        leftDrive.reinit(
            motion.targetTurns() +
            motion.leftHoldOffset(),
            ODRIVE_BOOT_DELAY);

        Telemetry::logOdriveReady("LEFT");
    }

    if (!leftNow)
    {
        leftDrive.resetClosedLoopFlag();
    }

    leftAlivePrev = leftNow;

    // --------------------------------------------------------
    // RIGHT
    // --------------------------------------------------------

    bool rightNow = rightDrive.isAlive();

    if (rightNow && !rightAlivePrev)
    {
        Telemetry::logOdrivePowerRestored("RIGHT");

        Telemetry::logOdriveReinit("RIGHT");

        rightDrive.reinit(
            -motion.targetTurns() +
            motion.rightHoldOffset(),
            ODRIVE_BOOT_DELAY);

        Telemetry::logOdriveReady("RIGHT");
    }

    if (!rightNow)
    {
        rightDrive.resetClosedLoopFlag();
    }

    rightAlivePrev = rightNow;
}

// ============================================================
// SETUP
// ============================================================

void setup()
{
    Serial.begin(115200);

    Serial.println();
    Serial.println("================================");
    Serial.println("ELLIC TURN CONTROL");
    Serial.println("================================");

    // --------------------------------------------------------
    // AS5600
    // --------------------------------------------------------

    if (!valEncoder.begin(SDA_PIN, SCL_PIN))
    {
        Serial.println("AS5600 / MAGNET ERROR");

        while (true)
        {
            delay(1000);
        }
    }

    // --------------------------------------------------------
    // BRAKES / PHASE
    // --------------------------------------------------------

    phase.begin(
        LEFT_BRAKE_PIN,
        RIGHT_BRAKE_PIN);

    // --------------------------------------------------------
    // ODRIVE UART
    // --------------------------------------------------------

    leftDrive.begin(
        115200,
        LEFT_RX,
        LEFT_TX);

    rightDrive.begin(
        115200,
        RIGHT_RX,
        RIGHT_TX);

    delay(1000);

    // --------------------------------------------------------
    // ENCODER BASELINE
    // --------------------------------------------------------

    valEncoder.resetBaseline();

    // --------------------------------------------------------
    // MOTION CONTROL
    // --------------------------------------------------------

    motion.begin();

    // --------------------------------------------------------
    // BLE
    // --------------------------------------------------------

    BLEconnect_begin();

    // --------------------------------------------------------
    // TELEMETRY
    // --------------------------------------------------------

    telemetry.begin();

    Serial.println("READY");
}

// ============================================================
// LOOP
// ============================================================

void loop()
{
    // --------------------------------------------------------
    // BLE GAMEPAD
    // --------------------------------------------------------

    BLEconnect_update();

    // --------------------------------------------------------
    // ENCODER
    // --------------------------------------------------------

    valEncoder.update();

    // --------------------------------------------------------
    // PHASE / BRAKES
    // --------------------------------------------------------

    phase.update();

    // --------------------------------------------------------
    // MOTOR CONTROL
    // --------------------------------------------------------

    motion.update();

    // --------------------------------------------------------
    // TELEMETRY CONTEXT
    // --------------------------------------------------------

    telemetry.setContext(
        valEncoder.rawAngle(),
        motion.targetTurns(),

        phase.leftBrake(),
        phase.rightBrake(),

        PhaseDetector::inTurnZone(
            valEncoder.rawAngle()),

        motion.isTurning(),

        motion.leftHoldOffset(),
        motion.rightHoldOffset());

    // --------------------------------------------------------
    // TELEMETRY COLLECTION
    // --------------------------------------------------------

    telemetry.update();

    // --------------------------------------------------------
    // COMPLETE TELEMETRY SAMPLE
    // --------------------------------------------------------

    if (telemetry.sampleReady())
    {
        // Передача полного пакета по BLE
        BLEconnect_sendTelemetry(
            telemetry.sample());

        // Проверка состояния ODrive
        handleAliveEdges();
    }
}