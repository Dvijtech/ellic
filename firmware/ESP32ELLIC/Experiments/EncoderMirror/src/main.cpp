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


// тормоза
constexpr int LEFT_BRAKE_PIN  = 32;
constexpr int RIGHT_BRAKE_PIN = 33;


constexpr float GEAR_RATIO = 10.4f;


// скорость поворота
constexpr float TURN_STEP = 0.01f;


constexpr uint32_t CONTROL_PERIOD = 5;
constexpr uint32_t PRINT_PERIOD = 100;


//====================================================
//                  ПЕРЕМЕННЫЕ
//====================================================

float rawAngle = 0.0f;
float lastAngle = 0.0f;

float delta = 0.0f;

float continuousAngle = 0.0f;

float targetTurns = 0.0f;


// смещение для поворота
float leftTurnOffset = 0;
float rightTurnOffset = 0;


bool leftActive = false;
bool rightActive = false;


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

bool inTurnZone(float angle)
{
    if (angle >= 350.0f)
        return true;

    if (angle <= 10.0f)
        return true;

    if (angle >= 170.0f && angle <= 190.0f)
        return true;

    return false;
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

void setIdle(HardwareSerial &port)
{
    port.println(
        "w axis0.requested_state 1"
    );
}


//====================================================

void setClosedLoop(HardwareSerial &port)
{
    port.println(
        "w axis0.requested_state 8"
    );
}


//====================================================

void setup()
{
    Serial.begin(115200);

    Serial.println();
    Serial.println("==============================");
    Serial.println("ELLIC TURN TEST");
    Serial.println("==============================");


    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(100000);


    delay(300);


    if (!encoder.begin())
    {
        Serial.println("AS5600 ERROR");

        while(true)
            delay(1000);
    }


    if (!encoder.magnetDetected())
    {
        Serial.println("MAGNET ERROR");

        while(true)
            delay(1000);
    }


    Serial.println("AS5600 OK");


    pinMode(
        LEFT_BRAKE_PIN,
        INPUT_PULLDOWN
    );

    pinMode(
        RIGHT_BRAKE_PIN,
        INPUT_PULLDOWN
    );


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


    Serial.print("START ANGLE: ");
    Serial.println(rawAngle);


    setClosedLoop(LeftODrive);
    setClosedLoop(RightODrive);


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



    bool leftBrake =
        digitalRead(LEFT_BRAKE_PIN);


    bool rightBrake =
        digitalRead(RIGHT_BRAKE_PIN);


    bool zone =
        inTurnZone(rawAngle);



    if (millis() - lastControl >= CONTROL_PERIOD)
    {

        lastControl = millis();



        //--------------------------------------------
        // ОБА ТОРМОЗА
        //--------------------------------------------

        if(leftBrake && rightBrake)
        {
            setIdle(LeftODrive);
            setIdle(RightODrive);

            leftTurnOffset = 0;
            rightTurnOffset = 0;
        }


        //--------------------------------------------
        // ЛЕВЫЙ ТОРМОЗ
        //--------------------------------------------

        else if(leftBrake)
        {

            if(zone)
            {
                setClosedLoop(RightODrive);

                rightTurnOffset += TURN_STEP;


                sendPosition(
                    RightODrive,
                    rightTurnOffset
                );


                setIdle(LeftODrive);
            }
            else
            {
                setIdle(LeftODrive);
                setIdle(RightODrive);
            }

        }


        //--------------------------------------------
        // ПРАВЫЙ ТОРМОЗ
        //--------------------------------------------

        else if(rightBrake)
        {

            if(zone)
            {

                setClosedLoop(LeftODrive);

                leftTurnOffset += TURN_STEP;


                sendPosition(
                    LeftODrive,
                    leftTurnOffset
                );


                setIdle(RightODrive);
            }
            else
            {
                setIdle(LeftODrive);
                setIdle(RightODrive);
            }

        }


        //--------------------------------------------
        // НОРМАЛЬНЫЙ РЕЖИМ
        //--------------------------------------------

        else
        {

            leftTurnOffset = 0;
            rightTurnOffset = 0;


            setClosedLoop(LeftODrive);
            setClosedLoop(RightODrive);


            sendPosition(
                LeftODrive,
                targetTurns
            );


            sendPosition(
                RightODrive,
                -targetTurns
            );

        }

    }



    if(millis() - lastPrint >= PRINT_PERIOD)
    {

        lastPrint = millis();


        Serial.print("RAW:");
        Serial.print(rawAngle,2);

        Serial.print(" TARGET:");
        Serial.print(targetTurns,3);

        Serial.print(" LB:");
        Serial.print(leftBrake);

        Serial.print(" RB:");
        Serial.print(rightBrake);

        Serial.print(" Z:");
        Serial.println(zone);

    }


    lastAngle = rawAngle;

}