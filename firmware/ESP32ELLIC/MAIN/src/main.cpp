#include <Arduino.h>

#include "Encoder.h"
#include "ODriveUART.h"
#include "MotionController.h"

// ============================================================
// PINS
// ============================================================

constexpr int ENCODER_SDA_PIN = 21;
constexpr int ENCODER_SCL_PIN = 22;

constexpr int LEFT_RX_PIN = 4;
constexpr int LEFT_TX_PIN = 5;

constexpr int RIGHT_RX_PIN = 26;
constexpr int RIGHT_TX_PIN = 25;

constexpr int LEFT_BRAKE_PIN = 32;
constexpr int RIGHT_BRAKE_PIN = 33;

// ============================================================
// UART
// ============================================================

constexpr uint32_t ODRIVE_BAUD = 115200;

// ============================================================
// CONTROL
// ============================================================

constexpr uint32_t CONTROL_PERIOD_MS = 100;

// ============================================================
// HARDWARE OBJECTS
// ============================================================

HardwareSerial LeftSerial(1);
HardwareSerial RightSerial(2);

Encoder encoder;

ODriveUART leftODrive(
    LeftSerial,
    "LEFT"
);

ODriveUART rightODrive(
    RightSerial,
    "RIGHT"
);

MotionController motionController(
    encoder,
    leftODrive,
    rightODrive
);

// ============================================================
// CONTROL TIMER
// ============================================================

uint32_t lastControlTime = 0;

// ============================================================
// SETUP
// ============================================================

void setup()
{
    Serial.begin(115200);

    delay(500);

    Serial.println();
    Serial.println("================================");
    Serial.println("ELLIC MOTION CONTROL");
    Serial.println("================================");

    // --------------------------------------------------------
    // ENCODER
    // --------------------------------------------------------

    Serial.println("1 ENCODER BEGIN");

    if (!encoder.begin(
        ENCODER_SDA_PIN,
        ENCODER_SCL_PIN
    ))
    {
        Serial.println("ENCODER ERROR");

        while (true)
        {
            delay(1000);
        }
    }

    Serial.println("2 ENCODER OK");

    // --------------------------------------------------------
    // LEFT ODRIVE
    // --------------------------------------------------------

    Serial.println("3 LEFT UART BEGIN");

    leftODrive.begin(
        ODRIVE_BAUD,
        LEFT_RX_PIN,
        LEFT_TX_PIN
    );

    Serial.println("4 LEFT UART OK");

    // --------------------------------------------------------
    // RIGHT ODRIVE
    // --------------------------------------------------------

    Serial.println("5 RIGHT UART BEGIN");

    rightODrive.begin(
        ODRIVE_BAUD,
        RIGHT_RX_PIN,
        RIGHT_TX_PIN
    );

    Serial.println("6 RIGHT UART OK");

    // --------------------------------------------------------
    // ODRIVE CONFIGURATION
    // --------------------------------------------------------

    Serial.println("7 ODRIVE CONFIG BEGIN");

    leftODrive.configure();
    rightODrive.configure();

    Serial.println("8 ODRIVE CONFIG OK");

    // --------------------------------------------------------
    // MOTION
    // --------------------------------------------------------

    Serial.println("9 MOTION BEGIN");

    motionController.begin(
        LEFT_BRAKE_PIN,
        RIGHT_BRAKE_PIN
    );

    Serial.println("10 MOTION OK");

    // --------------------------------------------------------
    // READY
    // --------------------------------------------------------

    Serial.println();
    Serial.println("================================");
    Serial.println("ELLIC READY");
    Serial.println("================================");

    lastControlTime = millis();
}

// ============================================================
// LOOP
// ============================================================

void loop()
{
    // --------------------------------------------------------
    // ENCODER
    // --------------------------------------------------------

    encoder.update();

    // --------------------------------------------------------
    // CONTROL
    // --------------------------------------------------------

    uint32_t now = millis();

    if (
        now - lastControlTime >=
        CONTROL_PERIOD_MS
    )
    {
        lastControlTime +=
            CONTROL_PERIOD_MS;

        motionController.update();
    }
}