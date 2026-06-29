#include <Arduino.h>
#include <Wire.h>
#include <AS5600.h>

AS5600 encoder;

void setup()
{
    Serial.begin(115200);

    Wire.begin(21,22);

    if (!encoder.begin())
    {
        Serial.println("Encoder NOT found");
        while(true);
    }

    Serial.println("Encoder OK");
}

void loop()
{
    float angle = encoder.readAngle() * 360.0 / 4096.0;

    Serial.println(angle);

    delay(20);
}