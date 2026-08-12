#pragma once

#include <Arduino.h>
#include "Encoder.h"
#include "PhaseDetector.h"
#include "MotionController.h"

// Весь Serial.print собран здесь, чтобы не мешать логике в других файлах
class Debug
{
public:
    void begin(uint32_t printPeriodMs);

    // Периодический вывод состояния, вызывать в каждой итерации loop()
    void update(const Encoder &encoder,
                const PhaseDetector &phase,
                const MotionController &motion);

    static void log(const char *msg);
    static void logOdriveReinit(const char *name);
    static void logOdriveReady(const char *name);
    static void logOdrivePowerRestored(const char *name);

private:
    uint32_t _printPeriodMs = 100;
    unsigned long _lastPrint = 0;
};