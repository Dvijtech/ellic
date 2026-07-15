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
//                   PIN CONFIG
//====================================================

constexpr int SDA_PIN = 21;
constexpr int SCL_PIN = 22;

constexpr int LEFT_RX  = 16;
constexpr int LEFT_TX  = 17;

constexpr int RIGHT_RX = 26;
constexpr int RIGHT_TX = 25;


// тормозные ручки

constexpr int LEFT_BRAKE_PIN  = 32;
constexpr int RIGHT_BRAKE_PIN = 33;


//====================================================
//                   SETTINGS
//====================================================

constexpr float GEAR_RATIO = 10.4f;

constexpr float TURN_STEP = 0.03f;


constexpr uint32_t CONTROL_PERIOD = 5;
constexpr uint32_t PRINT_PERIOD = 100;


//====================================================
//                   VARIABLES
//====================================================

float rawAngle = 0;
float lastAngle = 0;

float delta = 0;

float continuousAngle = 0;

float targetTurns = 0;


// временный поворотный offset

float leftTurnOffset = 0;
float rightTurnOffset = 0;


// сохраненный offset после поворота

float leftBaseOffset = 0;
float rightBaseOffset = 0;


// состояние поворота

bool turning = false;


// состояние ODrive

bool leftClosedLoop = false;
bool rightClosedLoop = false;



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

    if(d > 180)
        d -= 360;

    if(d < -180)
        d += 360;

    return d;
}


//====================================================

bool inTurnZone(float angle)
{
    if(angle >= 350)
        return true;

    if(angle <= 10)
        return true;

    if(angle >= 170 && angle <= 190)
        return true;

    return false;
}


//====================================================

void sendPosition(
    HardwareSerial &port,
    float turns
)
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

void setIdle(
    HardwareSerial &port
)
{
    port.println(
        "w axis0.requested_state 1"
    );
}


//====================================================

void setClosedLoop(
    HardwareSerial &port
)
{
    port.println(
        "w axis0.requested_state 8"
    );
}


//====================================================

void enableLeft()
{
    if(!leftClosedLoop)
    {
        setClosedLoop(LeftODrive);
        leftClosedLoop = true;
    }
}


//====================================================

void enableRight()
{
    if(!rightClosedLoop)
    {
        setClosedLoop(RightODrive);
        rightClosedLoop = true;
    }
}


//====================================================

void disableLeft()
{
    if(leftClosedLoop)
    {
        setIdle(LeftODrive);
        leftClosedLoop = false;
    }
}


//====================================================

void disableRight()
{
    if(rightClosedLoop)
    {
        setIdle(RightODrive);
        rightClosedLoop = false;
    }
}


//====================================================
// фиксация текущего положения после поворота
//====================================================

void finishTurn()
{

    leftBaseOffset += leftTurnOffset;

    rightBaseOffset += rightTurnOffset;


    leftTurnOffset = 0;
    rightTurnOffset = 0;


    turning = false;
}


//====================================================

void setup()
{

    Serial.begin(115200);


    Serial.println();
    Serial.println("==============================");
    Serial.println("ELLIC TURN CONTROL");
    Serial.println("==============================");


    Wire.begin(
        SDA_PIN,
        SCL_PIN
    );

    Wire.setClock(100000);


    delay(300);



    if(!encoder.begin())
    {
        Serial.println("AS5600 ERROR");

        while(true)
            delay(1000);
    }


    if(!encoder.magnetDetected())
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



    enableLeft();
    enableRight();



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
        digitalRead(
            LEFT_BRAKE_PIN
        );


    bool rightBrake =
        digitalRead(
            RIGHT_BRAKE_PIN
        );


    bool zone =
        inTurnZone(rawAngle);




    if(millis() - lastControl >= CONTROL_PERIOD)
    {

        lastControl = millis();



        //--------------------------------------------
        // оба тормоза
        //--------------------------------------------

        if(leftBrake && rightBrake)
        {

            disableLeft();
            disableRight();

            turning = true;

        }



        //--------------------------------------------
        // левый тормоз
        //--------------------------------------------

        else if(leftBrake)
        {

            if(zone)
            {

                turning = true;


                disableLeft();
                enableRight();



                rightTurnOffset += TURN_STEP;



                sendPosition(
                    RightODrive,
                    -targetTurns
                    + rightBaseOffset
                    + rightTurnOffset
                );

            }
            else
            {

                disableLeft();
                disableRight();

            }

        }



        //--------------------------------------------
        // правый тормоз
        //--------------------------------------------

        else if(rightBrake)
        {

            if(zone)
            {

                turning = true;


                disableRight();
                enableLeft();



                leftTurnOffset += TURN_STEP;



                sendPosition(
                    LeftODrive,
                    targetTurns
                    + leftBaseOffset
                    + leftTurnOffset
                );

            }
            else
            {

                disableLeft();
                disableRight();

            }

        }



        //--------------------------------------------
        // обычный режим
        //--------------------------------------------

        else
        {

            if(turning)
            {
                finishTurn();
            }



            enableLeft();
            enableRight();



            sendPosition(
                LeftODrive,
                targetTurns
                + leftBaseOffset
            );


            sendPosition(
                RightODrive,
                -targetTurns
                + rightBaseOffset
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
        Serial.print(zone);


        Serial.print(" LO:");
        Serial.print(leftBaseOffset,3);


        Serial.print(" RO:");
        Serial.println(rightBaseOffset,3);

    }



    lastAngle = rawAngle;

}