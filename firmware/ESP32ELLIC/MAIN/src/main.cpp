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

constexpr int LEFT_RX = 4;
constexpr int LEFT_TX = 5;

constexpr int RIGHT_RX = 26;
constexpr int RIGHT_TX = 25;

// ============================================================
// MOTION PARAMETERS
// ============================================================

constexpr float GEAR_RATIO = 4.4f;
constexpr float TURN_STEP = 0.03f;

constexpr uint32_t CONTROL_PERIOD = 50;

constexpr uint32_t TELEMETRY_PERIOD = 150;
constexpr uint32_t PRINT_PERIOD = 100;

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
// SETUP
// ============================================================

void setup()
{
    Serial.begin(115200);

    Serial.println();
    Serial.println("================================");
    Serial.println("ELLIC MOTION CONTROL");
    Serial.println("================================");

    // ========================================================
    // ENCODER
    // ========================================================

    Serial.println("1 ENCODER BEGIN");

    if (!valEncoder.begin(
            SDA_PIN,
            SCL_PIN))
    {
        Serial.println("AS5600 / MAGNET ERROR");

        while (true)
        {
            delay(1000);
        }
    }

    Serial.println("2 ENCODER OK");

    // ========================================================
    // ODRIVE UART
    // ========================================================

    Serial.println("3 LEFT UART BEGIN");

    leftDrive.begin(
        115200,
        LEFT_RX,
        LEFT_TX);

    Serial.println("4 LEFT UART OK");

    Serial.println("5 RIGHT UART BEGIN");

    rightDrive.begin(
        115200,
        RIGHT_RX,
        RIGHT_TX);

    Serial.println("6 RIGHT UART OK");

    // ========================================================
    // MOTION CONTROLLER
    // ========================================================

    Serial.println("7 MOTION BEGIN");

    motion.begin();

    Serial.println("8 MOTION OK");

    // ========================================================
    // BLE
    // ========================================================

    Serial.println("9 BLE BEGIN");

    BLEconnect_begin();

    Serial.println("10 BLE OK");

    Serial.println();
    Serial.println("================================");
    Serial.println("ELLIC READY");
    Serial.println("================================");
}

// ============================================================
// LOOP
// ============================================================

void loop()
{
    // --------------------------------------------------------
    // ENCODER
    // --------------------------------------------------------

    valEncoder.update();

    // --------------------------------------------------------
    // MOTION CONTROLLER
    // --------------------------------------------------------

    motion.update();

    // --------------------------------------------------------
    // BLE
    // --------------------------------------------------------

    // BLEconnect_update();

    // --------------------------------------------------------
    // TELEMETRY
    // --------------------------------------------------------

    // telemetry.update();
}