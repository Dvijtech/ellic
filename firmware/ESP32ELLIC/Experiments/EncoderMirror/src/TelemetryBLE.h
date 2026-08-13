#pragma once
#include "TelemetryCollector.h"

class TelemetryBLE
{
public:
    void begin(); // вызывать ПОСЛЕ BLETrigger_begin() — переиспользует NimBLE-сервер геймпада
    void send(const TelemetrySample &s);

private:
    class NimBLECharacteristic *_char = nullptr;
};