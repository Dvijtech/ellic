#include <Arduino.h>
#include <Wire.h>

void setup()
{
    Serial.begin(115200);

    Wire.begin(21, 22);
    Wire.setClock(100000);

    Serial.println("I2C Scanner");

    for (uint8_t addr = 1; addr < 127; addr++)
    {
        Wire.beginTransmission(addr);

        if (Wire.endTransmission() == 0)
        {
            Serial.print("Found: 0x");
            Serial.println(addr, HEX);
        }
    }

    Serial.println("Done");
}

void loop()
{
}