#include <Arduino.h>
#include "config.h"
#include "joystick.h"

static int center = 2048;

static int lastButtonState = HIGH;
static unsigned long lastDebounceTime = 0; // добавляли с кнопкой надеюсь нигде не конфликтанет
const unsigned long debounceDelay = 50; // добавляли с кнопкой надеюсь нигде не конфликтанет
void joystickButtonInit() {
    pinMode(PIN_JOYSTICK_BUTTON, INPUT_PULLUP);
    Serial.println("Кнопка джойстика инициализирована");
}

void joystickInit() {
    pinMode(PIN_JOYSTICK_X, INPUT);

    delay(500); // стабилизация

    // авто-калибровка центра
    long sum = 0;
    for (int i = 0; i < 100; i++) {
        sum += analogRead(PIN_JOYSTICK_X);
        delay(5);
    }
    center = sum / 100;

    Serial.print("Joystick center: ");
    Serial.println(center);
}

Zone readJoystick() {

    int value = analogRead(PIN_JOYSTICK_X);

    int threshold = (Deadzone / 100.0) * 4095.0;

    if (value < center - threshold)
        return ZONE_LEFT;

    if (value > center + threshold)
        return ZONE_RIGHT;

    return ZONE_DEAD;
}

bool isJoystickButtonPressed() {
    static int lastState = HIGH;
    static unsigned long lastChangeTime = 0;
    static bool pressReported = false;
    
    int currentState = digitalRead(PIN_JOYSTICK_BUTTON);
    unsigned long now = millis();
    
    if (currentState != lastState) {
        lastChangeTime = now;
        lastState = currentState;
    }
    
    if ((now - lastChangeTime) > 50) {  // Стабильно 50 мс
        if (currentState == LOW && !pressReported) {
            pressReported = true;
            return true;
        }
        if (currentState == HIGH) {
            pressReported = false;
        }
    }
    
    return false;
}

