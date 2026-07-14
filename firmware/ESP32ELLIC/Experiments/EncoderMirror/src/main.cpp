#include <Arduino.h>

#include "Encoder.h"
#include "ODriveUART.h"
#include "PhaseDetector.h"
#include "MotionController.h"


Encoder encoder;
PhaseDetector phaseDetector;
MotionController motionController;



// ================================
// ODrive
// ================================

ODriveUART leftODrive(
    &Serial1,
    1.0
);


ODriveUART rightODrive(
    &Serial2,
    -1.0
);



// ================================
// настройки
// ================================

constexpr uint32_t CONTROL_PERIOD = 20;


// максимальный момент
// пока безопасно
constexpr float MAX_TORQUE = 0.15f;


// ================================

unsigned long lastControl = 0;



// сигмоида 0..1
float sigmoid(float x)
{
    return 1.0f /
    (1.0f + exp(-x));
}



// плавный импульс

float gaitAssist(float phase)
{

    // центр импульса
    // -1 ... 1

    return sigmoid(phase*8.0f);


}



// ================================


void setup()
{

    Serial.begin(115200);


    Serial.println();
    Serial.println("=================");
    Serial.println("ELLIC TORQUE TEST");
    Serial.println("=================");



    encoder.begin();



    leftODrive.begin(
        16,
        17
    );


    rightODrive.begin(
        26,
        25
    );



    delay(1000);



    Serial.println();
    Serial.println(
        "Stand on RIGHT LEG"
    );


    Serial.println(
        "Calibration in 5 sec"
    );


    delay(5000);



    encoder.calibrateZero();



    phaseDetector.begin(0);

    motionController.begin();

    Serial.println("READY");

}



// ================================


void loop()
{

    float rawAngle =
        encoder.readJointAngle();



    phaseDetector.update(
        rawAngle
    );



    Serial.print("ANGLE ");
    Serial.print(
        phaseDetector.jointAngle()
    );


    Serial.print(" PHASE ");



    GaitPhase phase =
        phaseDetector.phase();



    switch(phase)
    {

        case GaitPhase::STOP:
            Serial.println("STOP");
            break;


        case GaitPhase::LEFT_START:
            Serial.println("LEFT_START");
            break;


        case GaitPhase::LEFT_PUSH:
            Serial.println("LEFT_PUSH");
            break;


        case GaitPhase::RIGHT_START:
            Serial.println("RIGHT_START");
            break;


        case GaitPhase::RIGHT_PUSH:
            Serial.println("RIGHT_PUSH");
            break;

    }





    if(
      millis()-lastControl 
      > CONTROL_PERIOD
    )
    {

        lastControl =
            millis();



        motionController.update(
            phaseDetector.jointAngle(),
            phaseDetector.phase()
        );


        leftODrive.setTorque(
            motionController.leftTorque()
        );


        rightODrive.setTorque(
            motionController.rightTorque()
        );

        Serial.print(
            " TORQUE "
        );

        Serial.print(
            motionController.leftTorque()
        );

        Serial.print(" ");

        Serial.println(
            motionController.rightTorque()
        );


    }


}