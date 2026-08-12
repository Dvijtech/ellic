#include <HardwareSerial.h>

HardwareSerial ODriveLeft(1);
HardwareSerial ODriveRight(2);


void setup() {

  Serial.begin(115200);

  // UART1
  ODriveLeft.begin(
      115200,
      SERIAL_8N1,
      16,
      17
  );


  // UART2
  ODriveRight.begin(
      115200,
      SERIAL_8N1,
      26,
      25
  );


  delay(1000);

  Serial.println("ESP32 started");

}


void loop() {

}