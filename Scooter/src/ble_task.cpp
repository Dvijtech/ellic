#include "ble_task.h"
#include "ble_gamepad.h"
#include "config.h"
#include <Arduino.h>

static QueueHandle_t buttonQueue;

// Задача, которая будет работать на ядре 1
void bleTask(void *parameter) {
    initBLEGamepad();
    
    uint8_t buttonState = 0;
    
    while (true) {
        // Ждём сообщение о нажатии кнопки (ждём максимум 10 мс)
        if (xQueueReceive(buttonQueue, &buttonState, pdMS_TO_TICKS(10)) == pdTRUE) {
            if (buttonState == 1) {
                sendGamepadButtonPress();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));  // Небольшая пауза
    }
}

void startBLETask() {
    // Создаём очередь для передачи нажатий
    buttonQueue = xQueueCreate(5, sizeof(uint8_t));
    
    // Запускаем задачу на ядре 1 (ядро 0 занято Wi-Fi и системой)
    xTaskCreatePinnedToCore(
        bleTask,    // Функция задачи
        "BLE Task", // Имя задачи
        4096,       // Стек
        NULL,       // Параметры
        1,          // Приоритет
        NULL,       // Handle
        1           // Ядро 1
    );
}

void notifyBLEButtonPress() {
    uint8_t press = 1;
    xQueueSend(buttonQueue, &press, 0);
}