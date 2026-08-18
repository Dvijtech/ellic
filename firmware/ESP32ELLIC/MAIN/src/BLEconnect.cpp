#include "BLEconnect.h"

#include <BleGamepad.h>
#include <NimBLEDevice.h>

// ============================================================
// BLE GAMEPAD
// ============================================================

BleGamepad bleGamepad("ESP32 Steering Wheel", "DIY", 100);

// Пин кнопки SW на HW-504
constexpr int JOY_BTN_PIN = 27;

// Аналоговые оси HW-504
constexpr int JOY_X_PIN = 34;
constexpr int JOY_Y_PIN = 35;

// Антидребезг
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
constexpr unsigned long DEBOUNCE_DELAY = 25;

// ============================================================
// BLE TELEMETRY
// ============================================================

static const char *SERVICE_UUID =
    "a1b2c3d0-1000-4000-8000-00805f9b34fb";

static const char *CHAR_UUID =
    "a1b2c3d1-1000-4000-8000-00805f9b34fb";

NimBLECharacteristic *telemetryCharacteristic = nullptr;

// Формат пакета сохраняем полностью таким же,
// как был в старом TelemetryBLE.cpp.
#pragma pack(push, 1)

struct TelemetryPacket
{
    uint32_t tMs;

    float rawAngle;
    float targetTurns;

    float leftCurrent;
    float leftVoltage;
    float leftVelocity;

    float rightCurrent;
    float rightVoltage;
    float rightVelocity;

    uint8_t flags;
};

#pragma pack(pop)

// ============================================================
// BEGIN
// ============================================================

void BLEconnect_begin()
{
    pinMode(JOY_BTN_PIN, INPUT_PULLUP);
    analogReadResolution(12);

    bleGamepad.begin();

    // Ждём не просто создания сервера, а завершения инициализации геймпада
    while (NimBLEDevice::getServer() == nullptr)
        delay(10);

    NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();

    while (!adv->isAdvertising())
        delay(10);

    NimBLEServer *server = NimBLEDevice::getServer();

    // Останавливаем адвертайзинг на время добавления сервиса, чтобы никто
    // не успел подключиться и получить неполную GATT-таблицу.
    adv->stop();

    NimBLEService *service = server->createService(SERVICE_UUID);

    telemetryCharacteristic = service->createCharacteristic(
        CHAR_UUID,
        NIMBLE_PROPERTY::NOTIFY);

    // В NimBLE-Arduino 2.x регистрация идёт через server->start(),
    // а не через service->start() (тот теперь no-op).
    server->start();

    // Если клиент уже подключён/забондился раньше и держит устаревший
    // кэш GATT — просим его перечитать таблицу сервисов.
    server->sendServiceChangedIndication();

///    adv->addServiceUUID(SERVICE_UUID);
    adv->start();
}

// ============================================================
// UPDATE GAMEPAD
// ============================================================

void BLEconnect_update()
{
    if (!bleGamepad.isConnected())
        return;

    // --------------------------------------------------------
    // КНОПКА Y
    // --------------------------------------------------------

    bool rawButtonState = digitalRead(JOY_BTN_PIN);

    if (rawButtonState != lastButtonState &&
        (millis() - lastDebounceTime) > DEBOUNCE_DELAY)
    {
        lastDebounceTime = millis();
        lastButtonState = rawButtonState;

        // Нажатие = LOW
        if (rawButtonState == LOW)
        {
            bleGamepad.press(BUTTON_4);
        }
        else
        {
            bleGamepad.release(BUTTON_4);
        }
    }

    // --------------------------------------------------------
    // JOYSTICK HW-504
    // --------------------------------------------------------

    int xVal = analogRead(JOY_X_PIN);
    int yVal = analogRead(JOY_Y_PIN);

    // X: 0..4095 -> -32767..32767
    int16_t mappedX =
        map(xVal, 0, 4095, -32767, 32767);

    // Y инвертирован
    int16_t mappedY =
        map(yVal, 0, 4095, 32767, -32767);

    bleGamepad.setLeftThumb(mappedX, mappedY);
}

// ============================================================
// SEND TELEMETRY
// ============================================================

void BLEconnect_sendTelemetry(const TelemetrySample &s)
{
    if (!telemetryCharacteristic)
        return;

    // Собираем пакет в точно таком же формате,
    // как в старом TelemetryBLE.cpp.

    TelemetryPacket p
    {
        s.tMs,

        s.rawAngle,
        s.targetTurns,

        s.leftCurrent,
        s.leftVoltage,
        s.leftVelocity,

        s.rightCurrent,
        s.rightVoltage,
        s.rightVelocity,

        static_cast<uint8_t>(
            (s.leftBrake  ? 1 : 0) |
            (s.rightBrake ? 2 : 0) |
            (s.turnZone   ? 4 : 0) |
            (s.turning    ? 8 : 0))
    };

    telemetryCharacteristic->setValue(
        reinterpret_cast<uint8_t *>(&p),
        sizeof(p));

    telemetryCharacteristic->notify();
}