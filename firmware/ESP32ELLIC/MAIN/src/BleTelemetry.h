#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

class BLEServer;
class BLECharacteristic;

// BleTelemetry: единственный модуль, отвечающий за передачу текстового
// телеметрического потока по BLE. Дублирует ровно тот же текст, что
// Telemetry пишет в Serial (см. Telemetry::emit()) - сам текст не
// формирует и не хранит.
//
// КРИТИЧНО: BLECharacteristic::notify() может блокировать вызывающий поток на
// непредсказуемое время. Поэтому send() никогда не блокирует: текст кладётся в
// FreeRTOS-очередь (потеря при переполнении допустима), а реальная отправка
// notify() идёт в отдельной задаче на ядре 0, физически не способной
// задержать loop() (ядро 1). Управление моторами не зависит от Bluetooth.
//
// v3: в очередь кладётся ВЕСЬ пакет телеметрии (несколько строк, ~500 байт)
// одним элементом, а не по одной строке. Причина: BLE ограничен числом notify()
// в секунду (по одному на connection event), а не байтами; при 4-6 строках в
// пакете каждые 100 мс половина строк терялась. Теперь пакет режется на куски
// по BLE_CHUNK_SIZE байт (2-3 notify на пакет), а потери - только целыми
// пакетами, без оборванных строк.
//
// Профиль сервиса совместим с Nordic UART Service (NUS) - характеристика TX, notify.
//
// ВАЖНО: если в проекте будет снова включён EllicVRButton (BleGamepad),
// нельзя независимо поднимать два BLE-стека на одном ESP32 - оба модуля
// должны использовать один и тот же BLE-стек (см. main.cpp).
class BleTelemetry {
public:
    BleTelemetry();

    // Поднимает BLE GATT-сервер, запускает рекламу (advertising) и
    // отдельную задачу отправки на ядре 0.
    void begin(const char* deviceName = "ELLIC Telemetry");

    // Вызывать в каждом проходе loop() - лёгкая операция (проверка одного
    // флага), перезапускает рекламу после дисконнекта.
    void update();

    // Кладёт текст (несколько строк, разделённых '\n') во внутреннюю очередь
    // на отправку одним элементом. НИКОГДА не блокирует и не ждёт BLE: если
    // клиента нет или очередь переполнена - пакет просто теряется. Если текст
    // длиннее MAX_ITEM_LEN - обрезается по последнему '\n'.
    void send(const char* text);

    bool isConnected() const { return _connected; }

    // Вызываются из callback-класса подключения/отключения в .cpp.
    void handleConnect();
    void handleDisconnect();

private:
    // Максимальный размер одного элемента очереди (один пакет телеметрии).
    static const size_t MAX_ITEM_LEN = 800;
    static const size_t BLE_CHUNK_SIZE = 180;  // безопасно при MTU=247;
                                                // если заметите потери notify на
                                                // конкретном железе - уменьшите
    static const uint32_t BLE_CHUNK_GAP_MS = 5; // пауза между notify одного пакета
    static const int QUEUE_LENGTH = 6;          // ~1.2 с запаса при периоде 200 мс

    struct QueueItem {
        char data[MAX_ITEM_LEN];
        size_t len;
    };

    static void taskEntry(void* param);
    void taskLoop();

    BLEServer* _server;
    BLECharacteristic* _txCharacteristic;
    volatile bool _connected;
    volatile bool _needsAdvertisingRestart;

    QueueHandle_t _queue;
    TaskHandle_t _task;
};
