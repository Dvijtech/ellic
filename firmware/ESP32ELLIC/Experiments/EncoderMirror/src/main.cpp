#include <Arduino.h>
#include <Wire.h>
#include <AS5600.h>

AS5600 encoder;

// ---------------- ODrive ----------------

HardwareSerial &LeftODrive  = Serial1;
HardwareSerial &RightODrive = Serial2;

// ---------------- Encoder ----------------

float lastAcceptedAngle = 0.0f;
float continuousAngle = 0.0f;

const float DEAD_BAND = 15.0f;      // игнорировать изменения меньше 15°

float lastTargetTurns = 999999.0f;

//------------------------------------------------

void sendBoth(const String &cmd)
{
    LeftODrive.println(cmd);
    RightODrive.println(cmd);
}

//------------------------------------------------

float readAngle()
{
    return encoder.readAngle() * 360.0f / 4096.0f;
}

//------------------------------------------------

void setup()
{
    Serial.begin(115200);

    LeftODrive.begin(115200, SERIAL_8N1, 16, 17);
    RightODrive.begin(115200, SERIAL_8N1, 26, 25);

    Wire.begin(21,22);

    delay(1000);

    if(!encoder.begin())
    {
        Serial.println("AS5600 ERROR");
        while(true);
    }

    lastAcceptedAngle = readAngle();

    Serial.println();
    Serial.println("===== START =====");

    sendBoth("w axis0.requested_state 8");
    delay(300);

    sendBoth("w axis0.controller.config.control_mode 3");
    delay(100);

    sendBoth("w axis0.controller.config.input_mode 1");
    delay(100);

    sendBoth("w axis0.controller.input_pos 0");
    delay(300);

    Serial.println("READY");
}

//------------------------------------------------

void loop()
{
    static unsigned long lastPrint = 0;

    float angle = readAngle();

    float delta = angle - lastAcceptedAngle;

    if(delta > 180.0f) delta -= 360.0f;
    if(delta < -180.0f) delta += 360.0f;

    // Если меньше 15° — считаем, что ничего не произошло
    if(fabs(delta) >= DEAD_BAND)
    {
        continuousAngle += delta;
        lastAcceptedAngle = angle;

        float targetTurns = continuousAngle / 360.0f;

        // Отправляем только если реально изменилась цель
        if(targetTurns != lastTargetTurns)
        {
            lastTargetTurns = targetTurns;

            sendBoth(
                "w axis0.controller.input_pos " +
                String(targetTurns,4)
            );
        }
    }

    if(millis() - lastPrint >= 1000)
    {
        lastPrint = millis();

        Serial.println("-----------------------");
        Serial.print("RAW ANGLE      : ");
        Serial.println(angle,2);

        Serial.print("ACCEPTED ANGLE : ");
        Serial.println(lastAcceptedAngle,2);

        Serial.print("CONTINUOUS     : ");
        Serial.println(continuousAngle,2);

        Serial.print("TARGET TURNS   : ");
        Serial.println(lastTargetTurns,4);
    }

    delay(10);
}