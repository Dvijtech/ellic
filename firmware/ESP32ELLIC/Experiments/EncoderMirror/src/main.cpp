#include <Arduino.h>

#include "Encoder.h"
#include "ODriveUART.h"
#include "PhaseDetector.h"
#include "MotionController.h"
#include "TelemetryCollector.h"
#include "TelemetryBLE.h"
#include "Debug.h"
#include "BLETrigger.h"

constexpr int SDA_PIN = 21, SCL_PIN = 22;
constexpr int LEFT_RX = 16, LEFT_TX = 17;
constexpr int RIGHT_RX = 26, RIGHT_TX = 25;
constexpr int LEFT_BRAKE_PIN = 32, RIGHT_BRAKE_PIN = 33;

constexpr float GEAR_RATIO = 4.4f;
constexpr float TURN_STEP  = 0.03f;

constexpr uint32_t CONTROL_PERIOD = 50;
constexpr uint32_t PRINT_PERIOD   = 100;
constexpr uint32_t TELEMETRY_PERIOD = 150; // ~6-7 Гц
constexpr uint32_t ODRIVE_BOOT_DELAY = 1500;

Encoder valEncoder;
ODriveUART leftDrive(Serial1, "LEFT");
ODriveUART rightDrive(Serial2, "RIGHT");
PhaseDetector phase;
MotionController motion(valEncoder, leftDrive, rightDrive, phase, GEAR_RATIO, TURN_STEP, CONTROL_PERIOD);
TelemetryCollector telemetry(leftDrive, rightDrive, TELEMETRY_PERIOD);
TelemetryBLE telemetryBLE;
Debug debug;

bool leftAlivePrev = false, rightAlivePrev = false;

void handleAliveEdges()
{
    bool leftNow = leftDrive.isAlive();
    if (leftNow && !leftAlivePrev)
    {
        Debug::logOdrivePowerRestored("LEFT");
        Debug::logOdriveReinit("LEFT");
        leftDrive.reinit(motion.targetTurns() + motion.leftHoldOffset(), ODRIVE_BOOT_DELAY);
        Debug::logOdriveReady("LEFT");
    }
    if (!leftNow) leftDrive.resetClosedLoopFlag();
    leftAlivePrev = leftNow;

    bool rightNow = rightDrive.isAlive();
    if (rightNow && !rightAlivePrev)
    {
        Debug::logOdrivePowerRestored("RIGHT");
        Debug::logOdriveReinit("RIGHT");
        rightDrive.reinit(-motion.targetTurns() + motion.rightHoldOffset(), ODRIVE_BOOT_DELAY);
        Debug::logOdriveReady("RIGHT");
    }
    if (!rightNow) rightDrive.resetClosedLoopFlag();
    rightAlivePrev = rightNow;
}

void setup()
{
    Serial.begin(115200);
    Serial.println("ELLIC TURN CONTROL");

    if (!valEncoder.begin(SDA_PIN, SCL_PIN))
    {
        Serial.println("AS5600 / MAGNET ERROR");
        while (true) delay(1000);
    }

    phase.begin(LEFT_BRAKE_PIN, RIGHT_BRAKE_PIN);
    leftDrive.begin(115200, LEFT_RX, LEFT_TX);
    rightDrive.begin(115200, RIGHT_RX, RIGHT_TX);

    delay(1000);
    valEncoder.resetBaseline();

    motion.begin();
    debug.begin(PRINT_PERIOD);

    BLETrigger_begin();
    telemetryBLE.begin(); // после геймпада — переиспользует его NimBLE-сервер
    telemetry.begin();

    Serial.println("READY");
}

void loop()
{
    BLETrigger_update();

    valEncoder.update();
    phase.update();
    motion.update();

    telemetry.setContext(valEncoder.rawAngle(), motion.targetTurns(),
                          phase.leftBrake(), phase.rightBrake(),
                          PhaseDetector::inTurnZone(valEncoder.rawAngle()), motion.isTurning());
    telemetry.update();

    if (telemetry.sampleReady())
    {
        telemetryBLE.send(telemetry.sample());
        handleAliveEdges();
    }

    debug.update(valEncoder, phase, motion);
}