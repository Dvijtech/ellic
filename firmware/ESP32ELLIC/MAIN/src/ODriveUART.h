#pragma once
#include <Arduino.h>
#include "Config.h"

class Telemetry;

// ODriveUART: единственный модуль, обращающийся к UART одного ODrive.
// Содержит:
//  - автомат автоконфигурации (раздел 10.2),
//  - фоновую диагностику по кругу из 7 параметров (раздел 12.1),
//  - выполнение MoveWheel (f0 + p0, раздел 9 / 14.2.1),
//  - счётчики и online/failStreak (раздел 13).
//
// Приоритет задач (раздел 14.2.3) реализован не как явная FIFO-очередь,
// а через порядок вызовов в main.cpp (см. отчёт после кода):
//   1. moveWheel() вызывается напрямую из main.cpp раз в CONTROL_PERIOD_MS
//      и имеет право "достроить" ранее начатый (config/diag) обмен перед
//      отправкой собственного f0, но не начинает НОВЫЙ фоновый обмен.
//   2. update() (конфигурация, затем диагностика) вызывается в каждом loop().
class ODriveUART {
public:
    ODriveUART(HardwareSerial &serial, uint8_t rxPin, uint8_t txPin, const char* label);

    void setTelemetry(Telemetry* telemetry);

    // Открывает UART. Явный configure() из setup() ОТКЛЮЧЁН (раздел 10.1) -
    // соответствующего публичного метода в этом классе намеренно нет.
    void begin();

    // Вызывать в каждом проходе loop(): продвигает автомат конфигурации,
    // затем (если конфигурация завершена и UART свободен) - фоновую
    // диагностику (раздел 14.2.3: приоритет 2, затем 3).
    void update();

    // MoveWheel: f 0 -> currentPosition -> p 0 (currentPosition + delta).
    // Возвращает false, если f0 не получил ответа - в этом случае p0
    // для этого колеса в этом цикле не отправляется (раздел 9, 13).
    bool moveWheel(float delta);

    OdriveSnapshot getSnapshot() const;

private:
    enum class ConfigState {
        NotStarted,
        WaitingIdle,
        Configuring,
        ConfirmingClosedLoop,
        Done
    };

    // ---- автомат конфигурации (раздел 10.2) ----
    void updateConfigure();
    void sendConfigCommands();

    // ---- фоновая диагностика (раздел 12.1 / 12.3) ----
    void updateDiagnostics();
    void applyDiagResult(int index, const char* buf);

    // ---- низкоуровневый асинхронный обмен "один запрос в полёте" (14.2.2) ----
    void sendRequestAsync(const char* cmd);
    // 0 = ещё ждём, 1 = получена строка ответа (в _rxBuffer), -1 = ошибка/таймаут
    int  pollResponse();
    // Блокирующее ожидание уже отправленного запроса (используется, когда
    // moveWheel() должен дождаться завершения чужого обмена - раздел 14.2.3)
    bool waitForResponse(char* outBuf, size_t bufSize);
    void flushRx();
    void onSuccess();
    void onFailure();

    HardwareSerial &_serial;
    uint8_t _rxPin;
    uint8_t _txPin;
    const char* _label;
    Telemetry* _telemetry;

    // конфигурация
    ConfigState _configState;
    bool _confirmRequestSent;

    // асинхронный обмен
    bool _requestPending;
    uint32_t _requestSentMs;
    char _rxBuffer[64];
    size_t _rxBufferLen;

    // диагностика
    int _diagIndex;
    uint32_t _lastDiagRequestMs;

    // связь / статистика (раздел 13)
    int _failStreak;
    bool _online;
    uint32_t _txCount;
    uint32_t _rxCount;
    uint32_t _rxFailCount;

    // диагностический кэш (раздел 12.3)
    int _axisState;
    int _axisError;
    int _motorError;
    int _controllerError;
    float _Iq;
    float _Vq;
    float _velEstimate;
    uint32_t _diagnosticsTimestampMs;
};
