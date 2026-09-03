#include <Arduino.h>
#include <Wire.h>
#include "Encoder.h"
#include "MotionController.h"
#include "ODriveUART.h"
#include "Telemetry.h"

// AS5600 (I2C)
static const int AS5600_SDA_PIN = 21;
static const int AS5600_SCL_PIN = 22;
static const uint32_t I2C_CLOCK_HZ = 50000;

// LEFT ODrive UART
static const int LEFT_ODRIVE_RX_PIN = 4;
static const int LEFT_ODRIVE_TX_PIN = 5;
// RIGHT ODrive UART
static const int RIGHT_ODRIVE_RX_PIN = 26;
static const int RIGHT_ODRIVE_TX_PIN = 25;
static const uint32_t ODRIVE_BAUD = 115200;

Encoder encoder;
ODriveUART leftOdrive(Serial1, LEFT_ODRIVE_RX_PIN, LEFT_ODRIVE_TX_PIN, ODRIVE_BAUD, "LEFT");
ODriveUART rightOdrive(Serial2, RIGHT_ODRIVE_RX_PIN, RIGHT_ODRIVE_TX_PIN, ODRIVE_BAUD, "RIGHT");
MotionController motionController(encoder, leftOdrive, rightOdrive);
Telemetry telemetry(encoder, motionController, leftOdrive, rightOdrive);

uint32_t lastControlMs = 0;

void setup() {
    Serial.begin(115200);

    Wire.begin(AS5600_SDA_PIN, AS5600_SCL_PIN);
    Wire.setClock(I2C_CLOCK_HZ);

    encoder.setTelemetry(&telemetry);
    motionController.setTelemetry(&telemetry);
    leftOdrive.setTelemetry(&telemetry);
    rightOdrive.setTelemetry(&telemetry);

    encoder.begin();
    leftOdrive.begin();
    rightOdrive.begin();
    motionController.begin();
    telemetry.begin();

    telemetry.log(LogLevel::INFO, "main", "ELLIC system initialized");

    lastControlMs = millis();

    // Раздел 10.1: явный configure() из setup() отключён (не вызывается).
    // Конфигурация ODrive выполняется автоматически через updateConfigure() в loop().
}

void loop() {
    encoder.update();

    leftOdrive.update();
    rightOdrive.update();

    leftOdrive.updateConfigure();
    rightOdrive.updateConfigure();

    uint32_t now = millis();
    if (now - lastControlMs >= MotionController::CONTROL_PERIOD_MS) {
        lastControlMs = now;
        motionController.update();
    }

    telemetry.update();
}