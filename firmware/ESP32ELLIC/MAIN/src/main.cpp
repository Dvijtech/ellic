#include <Arduino.h>

#include "Encoder.h"
#include "MotionController.h"
#include "ODriveCAN.h"
#include "Telemetry.h"

// One ODriveCAN instance owns the complete ODrive CAN bus.
ODriveCAN odrive(nullptr);
Encoder encoder(nullptr);
MotionController motion(encoder, odrive, nullptr);
Telemetry telemetry(encoder, motion, odrive);

void setup() {
    Serial.begin(115200);
    delay(100);

    // Attach the telemetry sink after all objects have been constructed.
    encoder.setTelemetry(&telemetry);
    odrive.setTelemetry(&telemetry);

    telemetry.begin();
    telemetry.log(LogLevel::INFO, "main", "ELLIC startup");

    const bool encoderOk = encoder.begin();
    telemetry.log(encoderOk ? LogLevel::INFO : LogLevel::WARNING,
                  "main", encoderOk ? "Encoder initialized" : "Encoder initialization failed");

    const bool canOk = odrive.begin();
    telemetry.log(canOk ? LogLevel::INFO : LogLevel::CRITICAL,
                  "main", canOk ? "ODriveCAN initialized" : "ODriveCAN initialization failed");

    motion.begin();
    telemetry.log(LogLevel::INFO, "main", "MotionController initialized");
}

void loop() {
    encoder.update();
    odrive.update();
    odrive.updateConfigure();
    motion.update();
    telemetry.update();
}
