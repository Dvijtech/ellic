#include <Arduino.h>
#include <Wire.h>
#include <AS5600.h>

//====================================================
//                      AS5600
//====================================================

AS5600 encoder;

//====================================================
//                      ODrive
//====================================================

HardwareSerial &LeftODrive  = Serial1;
HardwareSerial &RightODrive = Serial2;

//====================================================
//                   НАСТРОЙКИ
//====================================================

constexpr int SDA_PIN = 21;
constexpr int SCL_PIN = 22;

constexpr int LEFT_RX  = 16;
constexpr int LEFT_TX  = 17;

constexpr int RIGHT_RX = 26;
constexpr int RIGHT_TX = 25;

constexpr float GEAR_RATIO = 10.4f;

// период отправки команд
constexpr uint32_t CONTROL_PERIOD = 20;

// период печати
constexpr uint32_t PRINT_PERIOD = 100;

//====================================================
//                  ПЕРЕМЕННЫЕ
//====================================================

float rawAngle = 0.0f;
float lastAngle = 0.0f;

float delta = 0.0f;

float continuousAngle = 0.0f;

float targetTurns = 0.0f;

unsigned long lastControl = 0;
unsigned long lastPrint = 0;

//====================================================

float readAngle()
{
    return encoder.readAngle() * 360.0f / 4096.0f;
}

//====================================================

float shortestDelta(float now, float old)
{
    float d = now - old;

    if (d > 180.0f)
        d -= 360.0f;

    if (d < -180.0f)
        d += 360.0f;

    return d;
}

//====================================================

void sendPosition(HardwareSerial &port, float turns)
{
    char buffer[64];

    snprintf(
        buffer,
        sizeof(buffer),
        "w axis0.controller.input_pos %.4f",
        turns
    );

    port.println(buffer);
}

//====================================================

void setup()
{
    Serial.begin(115200);

    Serial.println();
    Serial.println("==============================");
    Serial.println("ODRIVE POSITION MIRROR");
    Serial.println("==============================");

    //----------------------------------------

    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(100000);

    delay(300);

    if (!encoder.begin())
    {
        Serial.println("AS5600 ERROR");

        while (true)
            delay(1000);
    }

    if (!encoder.magnetDetected())
    {
        Serial.println("MAGNET ERROR");

        while (true)
            delay(1000);
    }

    Serial.println("AS5600 OK");

    //----------------------------------------

    LeftODrive.begin(
        115200,
        SERIAL_8N1,
        LEFT_RX,
        LEFT_TX
    );

    RightODrive.begin(
        115200,
        SERIAL_8N1,
        RIGHT_RX,
        RIGHT_TX
    );

    delay(1000);

    rawAngle = readAngle();
    lastAngle = rawAngle;

    Serial.print("START ANGLE : ");
    Serial.println(rawAngle,2);

    Serial.println("READY");
}

//====================================================

void loop()
{
    rawAngle = readAngle();

    delta = shortestDelta(
        rawAngle,
        lastAngle
    );

    continuousAngle += delta;

    targetTurns =
        continuousAngle *
        GEAR_RATIO /
        360.0f;

    if (millis() - lastControl >= CONTROL_PERIOD)
    {
        lastControl = millis();

        sendPosition(
            LeftODrive,
            targetTurns
        );

        sendPosition(
            RightODrive,
            -targetTurns
        );
    }

    if (millis() - lastPrint >= PRINT_PERIOD)
    {
        lastPrint = millis();

        Serial.print("RAW: ");
        Serial.print(rawAngle,2);

        Serial.print("  DELTA: ");
        Serial.print(delta,2);

        Serial.print("  CONT: ");
        Serial.print(continuousAngle,2);

        Serial.print("  TARGET: ");
        Serial.println(targetTurns,4);
    }

    lastAngle = rawAngle;
}