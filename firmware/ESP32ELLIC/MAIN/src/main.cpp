#include <Arduino.h>
#include <Wire.h>
#include "Config.h"
#include "Encoder.h"
#include "MotionController.h"
#include "ODriveCAN.h"
#include "Telemetry.h"

// Раздел 0: main.cpp содержит один экземпляр ODriveCAN, обслуживающий оба
// логических канала (RIGHT node_id=1, LEFT node_id=2).
Encoder encoder;
MotionController motionController;
ODriveCAN odriveCAN;

Telemetry telemetry(&encoder, &motionController, &odriveCAN);

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

    odriveCAN.setTelemetry(&telemetry);
    odriveCAN.begin();

    // Раздел 10.1/10.2: явная ASCII-конфигурация ODrive из ESP32 не
    // выполняется. При работе по CAN она не предусмотрена в принципе -
    // соответствующего метода в ODriveCAN намеренно нет.

    telemetry.log(LogLevel::INFO, "main", "ELLIC system initialized (CAN)");

    lastControlMs = millis();
}

void loop() {
    // Раздел 11: encoder.update() - на каждом проходе, без периода.
    encoder.update();

    // Раздел 14.1: приём CAN-кадров и обновление кэша/online-статуса
    // выполняется на каждом проходе loop(), не блокируясь телеметрией.
    odriveCAN.update();

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

        // Раздел 9/14.2: дальше каналы независимы - недоступная/устаревшая
        // позиция одного колеса не блокирует отправку команды другому.
        odriveCAN.moveLeftWheel(motionController.getLeftWheelDelta());
        odriveCAN.moveRightWheel(motionController.getRightWheelDelta());
    }

    // Раздел 12: телеметрия - собственные периоды collect()/printScheduled().
    telemetry.update();
}
