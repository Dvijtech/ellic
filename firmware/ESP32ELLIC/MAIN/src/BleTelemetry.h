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
// КРИТИЧНО (см. обсуждение "моторы не сразу срабатывают"): BLECharacteristic::
// notify() может блокировать вызывающий поток на непредсказуемое время в
// зависимости от состояния BLE-соединения. Если бы send() вызывал notify()
// напрямую из loop(), это задерживало бы odriveCAN.update() - а значит и
// вовремя не обновлялась бы позиция WHEEL, она "протухала" бы
// (CAN_NODE_STALE_MS, раздел 13.1 спецификации), и moveWheelInternal()
// переставал бы слать Set Input Pos. То есть Bluetooth мог бы напрямую
// останавливать моторы.
//
// Поэтому send() никогда не блокирует: строка кладётся в FreeRTOS-очередь
// (некритичная потеря строки при переполнении - это нормально), а реальная
// отправка notify() происходит в отдельной задаче на ядре 0, физически
// не способной задержать loop() (который выполняется на ядре 1 - см.
// begin()). Управление моторами полностью независимо от состояния
// Bluetooth: подключён клиент, не подключён, тормозит ли notify() -
// на moveLeftWheel()/moveRightWheel() это больше не влияет.
//
// Профиль сервиса совместим с Nordic UART Service (NUS) - характеристика
// TX, notify.
//
// ВАЖНО: если в проекте будет снова включён EllicVRButton (BleGamepad),
// нельзя независимо поднимать два BLE-стека на одном ESP32 - оба модуля
// должны использовать один и тот же BLE-стек (см. main.cpp). Пока
// EllicVRButton закомментирован, конфликта нет.
class BleTelemetry {
public:
    BleTelemetry();

    // Поднимает BLE GATT-сервер, запускает рекламу (advertising) и
    // отдельную задачу отправки на ядре 0.
    void begin(const char* deviceName = "ELLIC Telemetry");

    // Вызывать в каждом проходе loop() - лёгкая операция (проверка одного
    // флага), перезапускает рекламу после дисконнекта.
    void update();

    // Кладёт текст (может содержать несколько строк, разделённых '\n') во
    // внутреннюю очередь на отправку. НИКОГДА не блокирует и не ждёт BLE:
    // если клиента нет или очередь переполнена - строка просто теряется.
    void send(const char* text);

    bool isConnected() const { return _connected; }

    // Вызываются из callback-класса подключения/отключения в .cpp.
    void handleConnect();
    void handleDisconnect();

private:
    // Максимальная длина одной строки, которая помещается в очередь целиком
    // (совпадает с самым длинным буфером в Telemetry.cpp - строка ODRIVE).
    static const size_t MAX_LINE_LEN = 220;
    static const size_t BLE_CHUNK_SIZE = 180; // безопасно при MTU=247 (см. begin());
                                               // если заметите потери notify на
                                               // конкретном железе - уменьшите до 20
    static const int QUEUE_LENGTH = 24;       // запас на случай кратковременных
                                               // пауз в самой BLE-задаче

    struct QueueItem {
        char data[MAX_LINE_LEN];
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
