#include <Arduino.h>
#include <Wire.h>
#include <AS5600.h>

AS5600 encoder;

HardwareSerial LeftODrive(1);
HardwareSerial RightODrive(2);

float startAngle;

float currentTorque = 999.0;

float getAngle()
{
    return encoder.readAngle() * 360.0f / 4096.0f;
}

void sendTorque(float torque)
{
    if (fabs(torque - currentTorque) < 0.0001)
        return;

    currentTorque = torque;

    String cmd = "w axis0.controller.input_torque ";
    cmd += String(torque, 3);

    LeftODrive.println(cmd);
    RightODrive.println(cmd);
}

void setup()
{
    Serial.begin(115200);

    LeftODrive.begin(115200, SERIAL_8N1, 16, 17);
    RightODrive.begin(115200, SERIAL_8N1, 26, 25);

    Wire.begin(21, 22);

    delay(1000);

    if (!encoder.begin())
    {
        Serial.println("AS5600 ERROR");
        while (1);
    }

    startAngle = getAngle();

    Serial.println();
    Serial.println("===== TORQUE FOLLOW TEST =====");

    Serial.print("START ANGLE: ");
    Serial.println(startAngle);

    // Closed Loop
    LeftODrive.println("w axis0.requested_state 8");
    RightODrive.println("w axis0.requested_state 8");

    delay(500);

    // Torque Control
    LeftODrive.println("w axis0.controller.config.control_mode 1");
    RightODrive.println("w axis0.controller.config.control_mode 1");

    LeftODrive.println("w axis0.controller.config.input_mode 1");
    RightODrive.println("w axis0.controller.config.input_mode 1");

    delay(500);

    sendTorque(0);
}

void loop()
{
    float angle = getAngle();

    float delta = angle - startAngle;

    if (delta > 180)
        delta -= 360;

    if (delta < -180)
        delta += 360;

    float torque = 0.0;

    if (delta > 3)
        torque = 0.02;

    if (delta < -3)
        torque = -0.02;

    sendTorque(torque);

    static unsigned long lastPrint = 0;

    if (millis() - lastPrint >= 1000)
    {
        Serial.println("----------------------");
        Serial.print("ANGLE : ");
        Serial.println(angle, 1);

        Serial.print("DELTA : ");
        Serial.println(delta, 1);

        Serial.print("TORQUE: ");
        Serial.println(torque, 3);

        lastPrint = millis();
    }

    delay(20);
}