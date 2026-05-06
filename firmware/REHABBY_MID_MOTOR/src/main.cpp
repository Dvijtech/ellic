#include <Arduino.h>

#define JOY_Y_PIN A1
#define MOTOR_PIN 10

int center = 489;
int deadZone = 20;

float currentPWM = 0;
float joyFiltered = 489;

bool active = false;
bool wasActive = false;

unsigned long startTime = 0;

int motorStartPWM = 450; // пинок
int motorRunPWM = 35; // нормальная работа
int motorMaxPWM = 35; // максимум

void setup() {
pinMode(MOTOR_PIN, OUTPUT);
Serial.begin(9600);
}

void loop() {
int joyY = analogRead(JOY_Y_PIN);

// фильтр
joyFiltered = joyFiltered * 0.7 + joyY * 0.3;

float delta = center - joyFiltered;

// гистерезис
if (delta > deadZone + 3) active = true;
if (delta < deadZone - 3) active = false;

int targetPWM = 0;

if (active) {

// старт
if (!wasActive) {
  startTime = millis();
  wasActive = true;
}

// первые 100 мс даём пинок
if (millis() - startTime < 700) {
  targetPWM = motorStartPWM;
} else {
  // дальше обычная кривая
  float norm = (delta - deadZone) / (center - deadZone);
  norm = constrain(norm, 0.0, 1.0);

  float shaped = norm * norm;

  targetPWM = motorRunPWM + shaped * (motorMaxPWM - motorRunPWM);
}
} else {
wasActive = false;
targetPWM = 0;
}

// ограничение
targetPWM = constrain(targetPWM, 0, motorMaxPWM);

// плавность
currentPWM += (targetPWM - currentPWM) * 0.15;

analogWrite(MOTOR_PIN, (int)currentPWM);

// отладка
Serial.print("Y: ");
Serial.print(joyFiltered);
Serial.print(" | PWM: ");
Serial.println(currentPWM);

delay(20);
}