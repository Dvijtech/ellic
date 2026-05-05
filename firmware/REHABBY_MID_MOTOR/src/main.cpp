#include <Arduino.h>

#define JOY_Y_PIN A1
#define MOTOR_PIN 10

// Калибровка (под тебя)
int center = 489;     // центр джойстика (ты измерил)
int deadZone = 20;    // мёртвая зона

int currentPWM = 0;

void setup() {
pinMode(MOTOR_PIN, OUTPUT);
Serial.begin(9600);
}

void loop() {
int joyY = analogRead(JOY_Y_PIN);

// считаем отклонение от центра
int delta = center - joyY;
// ВАЖНО: у тебя вверх = уменьшение значения (0 при полном газе)

int targetPWM = 0;

if (delta > deadZone) {
targetPWM = map(delta, deadZone, center, 0, 85);
} else {
targetPWM = 0;
}

// защита от мусора
targetPWM = constrain(targetPWM, 0, 85);

// плавный разгон (очень важно для JYQD)
currentPWM += (targetPWM - currentPWM) * 0.1;

analogWrite(MOTOR_PIN, currentPWM);

// отладка
Serial.print("Y: ");
Serial.print(joyY);
Serial.print(" | PWM: ");
Serial.println(currentPWM);

delay(1000);
}
