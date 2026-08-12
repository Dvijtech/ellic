#include <Arduino.h>

#include "Encoder.h"
#include "ODriveUART.h"
#include "PhaseDetector.h"
#include "MotionController.h"
#include "Debug.h"
#include "BLETrigger.h"

//====================================================
//                   PIN CONFIG
//====================================================

constexpr int SDA_PIN = 21;
constexpr int SCL_PIN = 22;

constexpr int LEFT_RX  = 16;
constexpr int LEFT_TX  = 17;

constexpr int RIGHT_RX = 26;
constexpr int RIGHT_TX = 25;

constexpr int LEFT_BRAKE_PIN  = 32;
constexpr int RIGHT_BRAKE_PIN = 33;

//====================================================
//                    SETTINGS
//====================================================

constexpr float GEAR_RATIO = 4.4f;
constexpr float TURN_STEP  = 0.03f;

constexpr uint32_t CONTROL_PERIOD = 50;
constexpr uint32_t PRINT_PERIOD   = 100;

constexpr uint32_t ODRIVE_CHECK_PERIOD = 1000;
constexpr uint32_t ODRIVE_BOOT_DELAY   = 1500;

//====================================================
//                    OBJECTS
//====================================================

Encoder valEncoder;

ODriveUART leftDrive(Serial1, "LEFT");
ODriveUART rightDrive(Serial2, "RIGHT");

PhaseDetector phase;

MotionController motion(valEncoder, leftDrive, rightDrive, phase,
                         GEAR_RATIO, TURN_STEP, CONTROL_PERIOD);

Debug debug;

//====================================================
//              ODrive connection watchdog
//====================================================

unsigned long lastOdriveCheck = 0;
bool leftAlivePrev = false;
bool rightAlivePrev = false;


enum class OdriveCheckPhase { Idle, Pinging };
OdriveCheckPhase odriveCheckPhase = OdriveCheckPhase::Idle;

void handleLeftPingResult()
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
}

void handleRightPingResult()
{
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

void checkOdriveConnections()
{
    if (odriveCheckPhase == OdriveCheckPhase::Idle)
    {
        if (millis() - lastOdriveCheck < ODRIVE_CHECK_PERIOD)
            return;

        lastOdriveCheck = millis();

        leftDrive.requestPing();
        rightDrive.requestPing();

        odriveCheckPhase = OdriveCheckPhase::Pinging;
        return;
    }

    // Фаза ожидания ответа — не блокирует loop(), просто опрашивает
    if (leftDrive.pollPing())
        handleLeftPingResult();

    if (rightDrive.pollPing())
        handleRightPingResult();

    if (!leftDrive.isPinging() && !rightDrive.isPinging())
        odriveCheckPhase = OdriveCheckPhase::Idle;
}   



//====================================================
//                     SETUP
//====================================================

void setup()
{
    Serial.begin(115200);

    Serial.println();
    Serial.println("==============================");
    Serial.println("ELLIC TURN CONTROL");
    Serial.println("==============================");

    if (!valEncoder.begin(SDA_PIN, SCL_PIN))
    {
        Serial.println("AS5600 / MAGNET ERROR");
        while (true) delay(1000);
    }

    Serial.println("AS5600 OK");

    phase.begin(LEFT_BRAKE_PIN, RIGHT_BRAKE_PIN);

    leftDrive.begin(115200, LEFT_RX, LEFT_TX);
    rightDrive.begin(115200, RIGHT_RX, RIGHT_TX);

    delay(1000);

    valEncoder.resetBaseline();

    motion.begin();

    debug.begin(PRINT_PERIOD);

    BLETrigger_begin();

    Serial.println("READY");
}

//====================================================
//                      LOOP
//====================================================

void loop()
{
    BLETrigger_update();

    checkOdriveConnections();

    valEncoder.update();
    phase.update();

    motion.update();

    debug.update(valEncoder, phase, motion);
}