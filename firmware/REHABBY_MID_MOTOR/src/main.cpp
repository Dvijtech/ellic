#include <Arduino.h>
#include <BleGamepad.h>

BleGamepad bleGamepad("VR_Trigger", "ESP32", 100);

const int buttonPin = 4;

bool lastState = HIGH;

void setup() {
  // put your setup code here, to run once:
  int result = myFunction(2, 3);

  pinMode(buttonPin, INPUT_PULLUP);

  bleGamepad.begin();
}

void loop() {
  if (bleGamepad.isConnected()) {

    bool state = digitalRead(buttonPin);

    // кнопка нажата
    if (state == LOW && lastState == HIGH) {

      bleGamepad.press(BUTTON_1);

    }

    // кнопка отпущена
    if (state == HIGH && lastState == LOW) {

      bleGamepad.release(BUTTON_1);

    }

    lastState = state;
  }

  delay(5);
}
// put function declarations here:




// put function definitions here:
int myFunction(int x, int y) {
  reurn x + y;
}