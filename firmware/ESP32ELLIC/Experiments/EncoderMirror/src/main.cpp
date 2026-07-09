#include <Arduino.h>
#include <Wire.h>
#include <AS5600.h>

AS5600 encoder;

HardwareSerial &LeftODrive = Serial1;
HardwareSerial &RightODrive = Serial2;


float centerAngle = 0;
unsigned long lastPrint = 0;


// отправка команды ODrive
void sendCommand(HardwareSerial &port, const char *name, String cmd)
{
    port.print(cmd);
    port.print('\n');

    Serial.print("[");
    Serial.print(name);
    Serial.print("] ");
    Serial.println(cmd);
}


// нормализация угла (-180 ... +180)
float angleDifference(float a, float b)
{
    float diff = a - b;

    if(diff > 180)
        diff -= 360;

    if(diff < -180)
        diff += 360;

    return diff;
}


void setup()
{
    Serial.begin(115200);


    // ODrive UART
    LeftODrive.begin(115200, SERIAL_8N1, 16,17);
    RightODrive.begin(115200, SERIAL_8N1, 26,25);


    // AS5600
    Wire.begin(21,22);


    delay(1000);


    if(!encoder.begin())
    {
        Serial.println("AS5600 NOT FOUND");
        while(true);
    }


    Serial.println("AS5600 OK");


    // запоминаем центр
    centerAngle = encoder.readAngle()*360.0/4096.0;

    Serial.print("Center=");
    Serial.println(centerAngle);



    // включаем ODrive
    sendCommand(
        LeftODrive,
        "LEFT",
        "w axis0.requested_state 8"
    );

    sendCommand(
        RightODrive,
        "RIGHT",
        "w axis0.requested_state 8"
    );


    delay(500);
}



void loop()
{

    float angle =
        encoder.readAngle()*360.0/4096.0;


    float error =
        angleDifference(angle, centerAngle);



    // мертвая зона вокруг центра
    if(abs(error)<5)
    {
        error=0;
    }


    // преобразуем угол в момент
    float torque = error / 90.0 * 0.03;


    // ограничение
    if(torque > 0.03)
        torque = 0.03;

    if(torque < -0.03)
        torque = -0.03;



    String cmd =
        "w axis0.controller.input_torque " 
        + String(torque,3);



    sendCommand(
        LeftODrive,
        "LEFT",
        cmd
    );


    sendCommand(
        RightODrive,
        "RIGHT",
        cmd
    );



    // вывод 2 раза в секунду
    if(millis()-lastPrint>500)
    {
        Serial.print("ANGLE=");
        Serial.print(angle);

        Serial.print(" ERROR=");
        Serial.print(error);

        Serial.print(" TORQUE=");
        Serial.println(torque);

        lastPrint=millis();
    }


    delay(50);
}