#include "TelemetryBLE.h"
#include <NimBLEDevice.h>

static const char *SERVICE_UUID = "a1b2c3d0-1000-4000-8000-00805f9b34fb";
static const char *CHAR_UUID    = "a1b2c3d1-1000-4000-8000-00805f9b34fb";

#pragma pack(push, 1)
struct TelemetryPacket
{
    uint32_t tMs;
    float rawAngle, targetTurns;
    float leftCurrent, leftVoltage, leftVelocity;
    float rightCurrent, rightVoltage, rightVelocity;
    uint8_t flags; // bit0 LB, bit1 RB, bit2 zone, bit3 turning
};
#pragma pack(pop) // sizeof = 37 байт

void TelemetryBLE::begin()
{
    while (NimBLEDevice::getServer() == nullptr) delay(10);

    NimBLEServer *server = NimBLEDevice::getServer();
    NimBLEService *service = server->createService(SERVICE_UUID);

    _char = service->createCharacteristic(CHAR_UUID, NIMBLE_PROPERTY::NOTIFY);
    service->start();
}

void TelemetryBLE::send(const TelemetrySample &s)
{
    if (!_char) return;

    TelemetryPacket p{
        s.tMs, s.rawAngle, s.targetTurns,
        s.leftCurrent, s.leftVoltage, s.leftVelocity,
        s.rightCurrent, s.rightVoltage, s.rightVelocity,
        (uint8_t)((s.leftBrake ? 1:0) | (s.rightBrake ? 2:0) | (s.turnZone ? 4:0) | (s.turning ? 8:0))
    };

    _char->setValue((uint8_t*)&p, sizeof(p));
    _char->notify();
}