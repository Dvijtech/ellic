#include <Arduino.h>
#include <Wire.h>
#include <AS5600.h>
#include "BLETrigger.h"

//====================================================
//                     AS5600
//====================================================

AS5600 encoder;

//====================================================
//                     ODrive
//====================================================

HardwareSerial &LeftODrive  = Serial1;
HardwareSerial &RightODrive = Serial2;

//====================================================
//                   PIN CONFIG
//====================================================

constexpr int SDA_PIN = 21;
constexpr int SCL_PIN = 22;

constexpr int LEFT_RX  = 16;
constexpr int LEFT_TX  = 17;

constexpr int RIGHT_RX = 26;
constexpr int RIGHT_TX = 25;

// Тормозные ручки
constexpr int LEFT_BRAKE_PIN  = 32;
constexpr int RIGHT_BRAKE_PIN = 33;

//====================================================
//                    SETTINGS
//====================================================

constexpr float GEAR_RATIO = 4.4f;
constexpr float TURN_STEP  = 0.03f;

constexpr uint32_t CONTROL_PERIOD = 50;
constexpr uint32_t PRINT_PERIOD   = 100;

//====================================================
//                    VARIABLES
//====================================================

// ---------- Вал (Val) ----------
float rawAngleV = 0;
float lastAngleV = 0;
float deltaAngleV = 0;
float continuousAngleV = 0;
float targetTurnsV = 0;

// ---------- Колеса (Wheel) ----------
float leftTurnOffsetW = 0;
float rightTurnOffsetW = 0;

float leftHoldOffsetW = 0;
float rightHoldOffsetW = 0;

float leftTargetW = 0;
float rightTargetW = 0;

bool turning = false;

bool leftClosedLoop = false;
bool rightClosedLoop = false;

unsigned long lastControl = 0;
unsigned long lastPrint = 0;

// Проверка состояния ODrive
bool leftODriveAlive = false;
bool rightODriveAlive = false;

constexpr uint32_t ODRIVE_CHECK_PERIOD = 1000;
constexpr uint32_t ODRIVE_BOOT_DELAY  = 1500;

unsigned long lastODriveCheck = 0;

void sendPosition(HardwareSerial &port, float turns);
void setClosedLoop(HardwareSerial &port);

bool checkODrive(HardwareSerial &port)
{
    while (port.available())
        port.read();

    port.println("r vbus_voltage");

    unsigned long start = millis();

    while (millis() - start < 100)
    {
        if (port.available())
        {
            String s = port.readStringUntil('\n');
            s.trim();

            if (s.length() > 0)
                return true;
        }
    }

    return false;
}

void reinitLeftODrive()
{
    Serial.println("LEFT ODrive REINIT");
    leftClosedLoop = false;

    delay(ODRIVE_BOOT_DELAY);

    float pos = targetTurnsV + leftHoldOffsetW;
    sendPosition(LeftODrive, pos);
    delay(50);

    setClosedLoop(LeftODrive);
    delay(100);

    leftClosedLoop = true;
    Serial.println("LEFT ODrive READY");
}

void reinitRightODrive()
{
    Serial.println("RIGHT ODrive REINIT");
    rightClosedLoop = false;

    delay(ODRIVE_BOOT_DELAY);

    float pos = -targetTurnsV + rightHoldOffsetW;
    sendPosition(RightODrive, pos);
    delay(50);

    setClosedLoop(RightODrive);
    delay(100);

    rightClosedLoop = true;
    Serial.println("RIGHT ODrive READY");
}

float readAngle()
{
    return encoder.readAngle() * 360.0f / 4096.0f;
}

float shortestDelta(float now, float old)
{
    float d = now - old;

    if(d > 180)  d -= 360;
    if(d < -180) d += 360;

    return d;
}

bool inTurnZone(float angle)
{
    if(angle >= 350 || angle <= 10) return true;
    if(angle >= 170 && angle <= 190) return true;

    return false;
}

void sendPosition(HardwareSerial &port, float turns)
{
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "w axis0.controller.input_pos %.4f", turns);
    port.println(buffer);
}

void setIdle(HardwareSerial &port)
{
    port.println("w axis0.requested_state 1");
}

void setClosedLoop(HardwareSerial &port)
{
    port.println("w axis0.requested_state 8");
}

void enableLeft()
{
    if(!leftClosedLoop)
    {
        setClosedLoop(LeftODrive);
        leftClosedLoop = true;
    }
}

void enableRight()
{
    if(!rightClosedLoop)
    {
        setClosedLoop(RightODrive);
        rightClosedLoop = true;
    }
}

void disableLeft()
{
    if(leftClosedLoop)
    {
        setIdle(LeftODrive);
        leftClosedLoop = false;
    }
}

void disableRight()
{
    if(rightClosedLoop)
    {
        setIdle(RightODrive);
        rightClosedLoop = false;
    }
}

void finishTurn()
{
    leftHoldOffsetW += leftTurnOffsetW;
    rightHoldOffsetW += rightTurnOffsetW;

    leftTurnOffsetW = 0;
    rightTurnOffsetW = 0;

    turning = false;
}

void setup()
{
    Serial.begin(115200);

    Serial.println();
    Serial.println("==============================");
    Serial.println("ELLIC TURN CONTROL");
    Serial.println("==============================");

    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(50000);

    delay(300);

    if(!encoder.begin())
    {
        Serial.println("AS5600 ERROR");
        while(true) delay(1000);
    }

    if(!encoder.magnetDetected())
    {
        Serial.println("MAGNET ERROR");
        while(true) delay(1000);
    }

    Serial.println("AS5600 OK");

    pinMode(LEFT_BRAKE_PIN, INPUT_PULLDOWN);
    pinMode(RIGHT_BRAKE_PIN, INPUT_PULLDOWN);

    LeftODrive.begin(115200, SERIAL_8N1, LEFT_RX, LEFT_TX);
    RightODrive.begin(115200, SERIAL_8N1, RIGHT_RX, RIGHT_TX);

    delay(1000);

    rawAngleV = readAngle();
    lastAngleV = rawAngleV;

    enableLeft();
    enableRight();

    BLETrigger_begin();

    Serial.println("READY");
}

void loop()
{
    BLETrigger_update();
    
    if (millis() - lastODriveCheck >= ODRIVE_CHECK_PERIOD)
    {
        lastODriveCheck = millis();

        bool leftNow = checkODrive(LeftODrive);
        bool rightNow = checkODrive(RightODrive);

        if (leftNow && !leftODriveAlive)
        {
            Serial.println("LEFT ODrive POWER RESTORED");
            reinitLeftODrive();
        }

        if (rightNow && !rightODriveAlive)
        {
            Serial.println("RIGHT ODrive POWER RESTORED");
            reinitRightODrive();
        }

        if (!leftNow)  leftClosedLoop = false;
        if (!rightNow) rightClosedLoop = false;

        leftODriveAlive = leftNow;
        rightODriveAlive = rightNow;
    }

    rawAngleV = readAngle();

    deltaAngleV = shortestDelta(rawAngleV, lastAngleV);
    continuousAngleV += deltaAngleV;

    targetTurnsV = continuousAngleV * GEAR_RATIO / 360.0f;

    bool leftBrake = digitalRead(LEFT_BRAKE_PIN);
    bool rightBrake = digitalRead(RIGHT_BRAKE_PIN);
    bool zone = inTurnZone(rawAngleV);

    if(millis() - lastControl >= CONTROL_PERIOD)
    {
        lastControl = millis();

        if(leftBrake && rightBrake)
        {
            disableLeft();
            disableRight();
            turning = true;
        }
        else if(leftBrake)
        {
            if(zone)
            {
                turning = true;
                disableLeft();
                enableRight();

                rightTurnOffsetW += TURN_STEP;
                rightTargetW = -targetTurnsV + rightHoldOffsetW + rightTurnOffsetW;
                sendPosition(RightODrive, rightTargetW);
            }
            else
            {
                disableLeft();
                disableRight();
            }
        }
        else if(rightBrake)
        {
            if(zone)
            {
                turning = true;
                disableRight();
                enableLeft();

                leftTurnOffsetW += TURN_STEP;
                leftTargetW = + targetTurnsV + leftHoldOffsetW + leftTurnOffsetW;
                sendPosition(LeftODrive, leftTargetW);
            }
            else
            {
                disableLeft();
                disableRight();
            }
        }
        else
        {
            if(turning)
            {
                finishTurn();
            }

            enableLeft();
            enableRight();

            leftTargetW = targetTurnsV + leftHoldOffsetW;
            sendPosition(LeftODrive, leftTargetW);

            rightTargetW = -targetTurnsV + rightHoldOffsetW;
            sendPosition(RightODrive, rightTargetW);
        }
    }

    if(millis() - lastPrint >= PRINT_PERIOD)
    {
        lastPrint = millis();

        Serial.print("RAW:");     Serial.print(rawAngleV,2);
        Serial.print(" TARGET:"); Serial.print(targetTurnsV,3);
        Serial.print(" LB:");     Serial.print(leftBrake);
        Serial.print(" RB:");     Serial.print(rightBrake);
        Serial.print(" Z:");      Serial.print(zone);
        Serial.print(" LO:");     Serial.print(leftHoldOffsetW,3);
        Serial.print(" RO:");     Serial.println(rightHoldOffsetW,3);
    }

    lastAngleV = rawAngleV;
}