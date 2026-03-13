#include <Arduino.h>
#include "config.h"
#include "joystick.h"
#include "motor.h"
#include "state_machine.h"
#include "ble_gamepad.h"  // НОВОЕ: подключаем наш геймпад

// ===== Global parameters =====
int MaxPower = 45;
int RampStep = 4;
int Deadzone = 30;
int Mode = 0;

unsigned long lastUpdate = 0;
unsigned long bootTime = 0;

bool systemArmed = false;

void setup() {  // Запускаем BLE на втором ядре
    pinMode(PIN_PWM_LEFT, OUTPUT);
    pinMode(PIN_PWM_RIGHT, OUTPUT);
    
    digitalWrite(PIN_PWM_LEFT, LOW);
    digitalWrite(PIN_PWM_RIGHT, LOW);

    delay(250);

    // Теперь остальная инициализация
    Serial.begin(115200);
    bootTime = millis();

    analogSetPinAttenuation(PIN_JOYSTICK_X, ADC_11db);

    joystickInit();
 
    motorInit();   // motorInit продублирует настройку PWM, но это безопасно
    
    
    joystickButtonInit();  // добавляем кнопку
    initBLEGamepad();        // добавляем клавиатуру блютус БЛЕ

}
void loop() {
    delay(1);

    Serial.println(analogRead(PIN_JOYSTICK_X));
    delay(200);

    if (millis() - bootTime < 500) { 
        setLeftMotor(0);
        setRightMotor(0);
        return;
    }

    if (millis() - lastUpdate >= LOOP_PERIOD_MS) {
        lastUpdate = millis();

        Zone zone = readJoystick();
        updateState(zone);
    }
    
    if (!systemArmed) {
        Zone zone = readJoystick();

        // Разрешаем движение только если джойстик в центре
        if (zone == ZONE_DEAD) {
            systemArmed = true;
        }

        setLeftMotor(0);
        setRightMotor(0);
        return;  // ← ВАЖНО: здесь return, поэтому код после if НЕ ВЫПОЛНЯЕТСЯ
    }
    
    // Обработка кнопки - теперь надёжно!
    if (isJoystickButtonPressed()) {
        Serial.println("Кнопка нажата!");
        sendGamepadButtonPress();
    }

}
