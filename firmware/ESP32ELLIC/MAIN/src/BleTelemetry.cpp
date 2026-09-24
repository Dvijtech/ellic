#include "BleTelemetry.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <string.h>

namespace {
// Nordic UART Service (NUS) UUID - стандартный, узнаваемый набор UUID для
// "BLE-как-UART". Выбран, чтобы поток при желании читался и готовыми
// приложениями, а не только нашей веб-страницей.
const char* SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
const char* CHAR_TX_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"; // ESP32 -> клиент, notify
const char* CHAR_RX_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"; // клиент -> ESP32,
                                                                    // зарезервировано под
                                                                    // будущие команды, сейчас
                                                                    // входящие данные игнорируются

// Основной loop() Arduino на ESP32 по умолчанию выполняется на ядре 1
// (APP_CPU). Задачу отправки телеметрии сознательно пиним на ядро 0
// (PRO_CPU), чтобы даже долгий notify() не отнимал время у ядра,
// на котором крутится управление моторами.
const BaseType_t BLE_TASK_CORE = 0;
const uint32_t   BLE_TASK_STACK_SIZE = 4096;
const UBaseType_t BLE_TASK_PRIORITY = 1; // ниже приоритета обычных Arduino-задач по умолчанию
} // namespace

class BleTelemetryServerCallbacks : public BLEServerCallbacks {
public:
    explicit BleTelemetryServerCallbacks(BleTelemetry* owner) : _owner(owner) {}

    void onConnect(BLEServer* /*server*/) override {
        _owner->handleConnect();
    }

    void onDisconnect(BLEServer* /*server*/) override {
        _owner->handleDisconnect();
    }

private:
    BleTelemetry* _owner;
};

BleTelemetry::BleTelemetry()
    : _server(nullptr),
      _txCharacteristic(nullptr),
      _connected(false),
      _needsAdvertisingRestart(false),
      _queue(nullptr),
      _task(nullptr) {}

void BleTelemetry::begin(const char* deviceName) {
    BLEDevice::init(deviceName);

    /// BLEDevice::setPower(ESP_PWR_LVL_N12); // минимальная мощность TX - снижает пиковый ток

    BLEDevice::setMTU(247); // запрашиваем увеличенный MTU у клиента, чтобы
                             // строки телеметрии не резались на 20 байт

    _server = BLEDevice::createServer();
    _server->setCallbacks(new BleTelemetryServerCallbacks(this));

    BLEService* service = _server->createService(SERVICE_UUID);

    _txCharacteristic = service->createCharacteristic(
        CHAR_TX_UUID,
        BLECharacteristic::PROPERTY_NOTIFY);
    _txCharacteristic->addDescriptor(new BLE2902());

    BLECharacteristic* rxCharacteristic = service->createCharacteristic(
        CHAR_RX_UUID,
        BLECharacteristic::PROPERTY_WRITE);
    (void)rxCharacteristic; // см. комментарий у CHAR_RX_UUID выше

    service->start();

    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(SERVICE_UUID);
    advertising->setScanResponse(true);
    advertising->start();

    // Очередь строк на отправку + отдельная задача, которая их реально шлёт
    // через notify(). Именно это отвязывает Bluetooth от основного loop() -
    // см. комментарий в BleTelemetry.h.
    _queue = xQueueCreate(QUEUE_LENGTH, sizeof(QueueItem));
    xTaskCreatePinnedToCore(
        &BleTelemetry::taskEntry,
        "BleTelemetryTx",
        BLE_TASK_STACK_SIZE,
        this,
        BLE_TASK_PRIORITY,
        &_task,
        BLE_TASK_CORE);
}

void BleTelemetry::update() {
    if (_needsAdvertisingRestart) {
        _needsAdvertisingRestart = false;
        BLEDevice::startAdvertising();
    }
}

void BleTelemetry::send(const char* text) {
    // Дешёвая проверка до похода в очередь: нет клиента - нет смысла
    // копировать и класть строку.
    if (!_connected || _queue == nullptr || text == nullptr) {
        return;
    }

    QueueItem item;
    size_t len = strlen(text);
    if (len >= MAX_LINE_LEN) {
        len = MAX_LINE_LEN - 1; // обрезаем, а не блокируемся и не выделяем память
    }
    memcpy(item.data, text, len);
    item.data[len] = '\0';
    item.len = len;

    // Таймаут 0: если очередь занята/полна - строка тихо теряется, но
    // loop() ни при каких обстоятельствах не ждёт.
    xQueueSend(_queue, &item, 0);
}

void BleTelemetry::handleConnect() {
    _connected = true;
}

void BleTelemetry::handleDisconnect() {
    _connected = false;
    _needsAdvertisingRestart = true;
}

void BleTelemetry::taskEntry(void* param) {
    static_cast<BleTelemetry*>(param)->taskLoop();
}

void BleTelemetry::taskLoop() {
    QueueItem item;
    for (;;) {
        // Блокировка тут безвредна - это отдельная задача на ядре 0,
        // а не поток, в котором крутится управление моторами.
        if (xQueueReceive(_queue, &item, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        if (!_connected || _txCharacteristic == nullptr) {
            continue; // клиент отключился, пока строка ждала в очереди
        }

        size_t offset = 0;
        while (offset < item.len) {
            size_t chunkLen = item.len - offset;
            if (chunkLen > BLE_CHUNK_SIZE) {
                chunkLen = BLE_CHUNK_SIZE;
            }
            _txCharacteristic->setValue((uint8_t*)(item.data + offset), chunkLen);
            _txCharacteristic->notify();
            offset += chunkLen;
        }
    }
}
