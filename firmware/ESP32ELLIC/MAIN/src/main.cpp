#include <Arduino.h>
#include <Wire.h>
#include "Config.h"
#include "Encoder.h"
#include "MotionController.h"
#include "ODriveUART.h"
#include "Telemetry.h"

// Раздел 0: main.cpp содержит ровно два экземпляра ODriveUART - LEFT и RIGHT.
Encoder encoder;
MotionController motionController;

HardwareSerial SerialOdriveLeft(1);
HardwareSerial SerialOdriveRight(2);

ODriveUART odriveLeft(SerialOdriveLeft, ODRIVE_LEFT_RX_PIN, ODRIVE_LEFT_TX_PIN, "LEFT");
ODriveUART odriveRight(SerialOdriveRight, ODRIVE_RIGHT_RX_PIN, ODRIVE_RIGHT_TX_PIN, "RIGHT");

Telemetry telemetry(&encoder, &motionController, &odriveLeft, &odriveRight);

uint32_t lastControlMs = 0;

void setup() {
    Serial.begin(115200);

    Wire.begin(AS5600_SDA_PIN, AS5600_SCL_PIN);
    Wire.setClock(AS5600_I2C_CLOCK_HZ);

    pinMode(LEFT_BRAKE_PIN, INPUT_PULLUP);
    pinMode(RIGHT_BRAKE_PIN, INPUT_PULLUP);

    telemetry.begin(LogLevel::INFO);

    encoder.begin(&telemetry);
    motionController.begin();

    odriveLeft.setTelemetry(&telemetry);
    odriveRight.setTelemetry(&telemetry);
    odriveLeft.begin();
    odriveRight.begin();

    // Раздел 10.1: явный configure() из setup() ОТКЛЮЧЁН.
    // leftODrive.configure();
    // rightODrive.configure();

    telemetry.log(LogLevel::INFO, "main", "ELLIC system initialized");

    lastControlMs = millis();
}

void loop() {
    // Раздел 11: encoder.update() - на каждом проходе, без периода.
    encoder.update();

    // Раздел 14: диспетчер каждого ODrive (конфигурация -> диагностика)
    // обрабатывается на каждом проходе loop().
    odriveLeft.update();
    odriveRight.update();

    uint32_t now = millis();
    if (now - lastControlMs >= CONTROL_PERIOD_MS) {
        lastControlMs = now;

        bool leftBrake  = (digitalRead(LEFT_BRAKE_PIN) == LOW);
        bool rightBrake = (digitalRead(RIGHT_BRAKE_PIN) == LOW);

        EncoderSnapshot encSnap = encoder.getSnapshot();

        // Разделы 6.2-8: общее решение по ОБОИМ тормозам + Val сразу
        // даёт оба приращения - leftWheelDelta и rightWheelDelta.
        motionController.update(encSnap.rawAngle, encSnap.continuousAngle,
                                 leftBrake, rightBrake);

        // Раздел 9/14: дальше каналы независимы - ошибка одного не
        // блокирует отправку команды другому.
        odriveLeft.moveWheel(motionController.getLeftWheelDelta());
        odriveRight.moveWheel(motionController.getRightWheelDelta());
    }

    // Раздел 12: телеметрия - собственные периоды collect()/printScheduled().
    telemetry.update();
}
