#include <Arduino.h>

void sendCommand(HardwareSerial &port, const char *cmd)
{
    port.print(cmd);
    port.print('\n');

    Serial.print(">> ");
    Serial.println(cmd);
}

void setup()
{
    Serial.begin(115200);

    // LEFT
    Serial1.begin(115200, SERIAL_8N1, 16, 17);

    // RIGHT
    Serial2.begin(115200, SERIAL_8N1, 26, 25);

    delay(2000);

    Serial.println("Starting ODrives");

    sendCommand(Serial1, "w axis0.requested_state 8");
    sendCommand(Serial2, "w axis0.requested_state 8");

    delay(500);
}

void loop()
{
    sendCommand(Serial1, "w axis0.controller.input_torque 0.02");
    sendCommand(Serial2, "w axis0.controller.input_torque 0.02");

    delay(2000);

    sendCommand(Serial1, "w axis0.controller.input_torque -0.02");
    sendCommand(Serial2, "w axis0.controller.input_torque -0.02");

    delay(2000);
}