#include <Arduino.h>
#include <Wire.h>
#include "Config.h"
#include "Encoder.h"
#include "MotionController.h"
#include "ODriveCAN.h"
#include "Telemetry.h"
#include "BleTelemetry.h"
#include "FireButton.h"
// #include "EllicVRButton.h"
// ВАЖНО: EllicVRButton (BleGamepad) и BleTelemetry - оба поднимают BLE-стек
// на ESP32. Пока EllicVRButton закомментирован, конфликта нет. Если его
// снова включат, нельзя просто раскомментировать: два независимых вызова
// инициализации BLE с высокой вероятностью конфликтуют (один BLE-радио на
// устройство) - оба модуля тогда нужно свести на один и тот же стек.

// Раздел 0: main.cpp содержит один экземпляр ODriveCAN, обслуживающий оба
// логических канала (RIGHT node_id=1, LEFT node_id=2).
Encoder encoder;
MotionController motionController;
ODriveCAN odriveCAN;

Telemetry telemetry(&encoder, &motionController, &odriveCAN);
BleTelemetry bleTelemetry;
FireButton fireButton;

// EllicVRButton vrButton("ELLIC VR Gamepad", 18);

uint32_t lastControlMs = 0;
uint32_t prevLoopStartUs = 0;

void setup() {
    Serial.begin(SERIAL_BAUD);
    Serial.printf("# ELLIC firmware %s, serial %lu baud\n", FW_VERSION, (unsigned long)SERIAL_BAUD);

    Wire.begin(AS5600_SDA_PIN, AS5600_SCL_PIN);
    Wire.setClock(AS5600_I2C_CLOCK_HZ);

    pinMode(LEFT_BRAKE_PIN, INPUT_PULLUP);
    pinMode(RIGHT_BRAKE_PIN, INPUT_PULLUP);

    telemetry.begin();

    fireButton.begin();
    telemetry.setFireButton(&fireButton);

    encoder.begin(&telemetry);
    motionController.begin();

    odriveCAN.setTelemetry(&telemetry);
    odriveCAN.begin();

    bleTelemetry.begin("ELLIC Telemetry");
    telemetry.setBleTelemetry(&bleTelemetry);

    // vrButton.begin();

    // Раздел 10: явная конфигурация ODrive из ESP32 не выполняется.
    // Параметры ODrive сохранены в самих платах (odrivetool).

    lastControlMs = millis();
}

void loop() {
    // Интервал между началами проходов loop() - для диагностики (запись D):
    // единичные длинные интервалы означают, что что-то блокировало loop().
    uint32_t loopStartUs = micros();
    if (prevLoopStartUs != 0) {
        telemetry.noteLoopGap(loopStartUs - prevLoopStartUs);
    }
    prevLoopStartUs = loopStartUs;

    // Кнопка стрельбы (раздел 13.2): состояние и счётчик уходят в периодическом
    // пакете; немедленная выдача включается флагом FIRE_SEND_IMMEDIATE.
    bool fireChanged = fireButton.update();
#if FIRE_SEND_IMMEDIATE
    if (fireChanged) {
        telemetry.sendFireEvent();
    }
#else
    (void)fireChanged;
#endif

    // Раздел 11: энкодер опрашивается не чаще ENCODER_PERIOD_MS (внутри update()).
    encoder.update();

    // Раздел 12.1: приём CAN-кадров, обновление кэша/online-статуса и
    // контроль состояния TWAI - на каждом проходе loop(), не блокируясь
    // телеметрией.
    odriveCAN.update();

    // Обслуживает переподключения BLE-клиента телеметрии.
    bleTelemetry.update();

    uint32_t now = millis();
    if (now - lastControlMs >= CONTROL_PERIOD_MS) {
        lastControlMs = now;

        bool leftBrake  = (digitalRead(LEFT_BRAKE_PIN) == LOW);
        bool rightBrake = (digitalRead(RIGHT_BRAKE_PIN) == LOW);

        EncoderSnapshot encSnap = encoder.getSnapshot();

        // Разделы 6.2-8: общее решение по ОБОИМ тормозам + Val сразу
        // даёт оба приращения - leftWheelDelta и rightWheelDelta.
        motionController.update(encSnap.rawAngle, encSnap.continuousAngle,
                                 leftBrake, rightBrake);

        // Раздел 9: дальше каналы независимы - недоступная/устаревшая
        // позиция одного колеса не блокирует отправку команды другому.
        odriveCAN.moveLeftWheel(motionController.getLeftWheelDelta());
        odriveCAN.moveRightWheel(motionController.getRightWheelDelta());
    }

    // Раздел 13: телеметрия - собственный период TELEMETRY_PERIOD_MS.
    telemetry.update();

    // vrButton.update();
}
