#ifndef ODRIVE_UART_H
#define ODRIVE_UART_H

#include <Arduino.h>

class Telemetry;

struct OdriveSnapshot {
    bool online;
    int axisState;
    int axisError;
    int motorError;
    int controllerError;
    float Iq;
    float Vq;
    float velEstimate;
    uint32_t txCount;
    uint32_t rxCount;
    uint32_t rxFailCount;
    uint32_t diagnosticsTimestampMs;
};

class ODriveUART {
public:
    static const uint32_t RESPONSE_TIMEOUT_MS   = 50;
    static const uint8_t  ALIVE_FAIL_THRESHOLD  = 5;

    ODriveUART(HardwareSerial &serial, int rxPin, int txPin, uint32_t baud, const char* label);

    void setTelemetry(Telemetry* telemetry);

    void begin();

    // Диспетчер очереди UART: MoveWheel / config-запросы / фоновая диагностика.
    // Вызывается на каждом проходе loop().
    void update();

    // Конечный автомат автоматической конфигурации (раздел 10.2).
    // Вызывается на каждом проходе loop().
    void updateConfigure();

    // Вызывается MotionController раз в CONTROL_PERIOD_MS.
    void requestMove(float wheelDelta);

    OdriveSnapshot getSnapshot() const;

private:
    enum class ConfigStage {
        NotStarted,
        WaitingIdle,
        Configuring,
        ConfirmingClosedLoop,
        Done
    };

    enum class Purpose {
        NONE,
        MOVE_READ_POS,
        CONFIG_CHECK_IDLE,
        CONFIG_CHECK_CLOSEDLOOP,
        DIAG_IQ,
        DIAG_VQ,
        DIAG_VEL,
        DIAG_STATE,
        DIAG_AXIS_ERR,
        DIAG_MOTOR_ERR,
        DIAG_CTRL_ERR
    };

    HardwareSerial &_serial;
    int _rxPin;
    int _txPin;
    uint32_t _baud;
    char _label[8];
    Telemetry* _telemetry;

    // --- низкоуровневый обмен: не более одного запроса "в полёте" ---
    bool _busy;
    Purpose _purpose;
    uint32_t _requestStartMs;
    String _rxBuffer;

    // --- MoveWheel ---
    bool _moveWheelPending;
    float _queuedWheelDelta;   // ждёт диспетчеризации
    float _activeWheelDelta;   // используется в момент разбора ответа f 0

    // --- конфигурация (раздел 10.2) ---
    ConfigStage _configStage;
    bool _configRequestPending;
    Purpose _configPendingPurpose;
    bool _configAwaitingResponse;
    bool _configResponseReady;
    bool _configResponseOk;
    float _configResponseValue;

    // --- фоновая диагностика ---
    uint8_t _diagIndex; // 0..6, round-robin
    float _Iq;
    float _Vq;
    float _velEstimate;
    int _axisState;
    int _axisError;
    int _motorError;
    int _controllerError;
    uint32_t _diagnosticsTimestampMs;

    // --- диагностика связи ---
    bool _online;
    uint8_t _failStreak;
    uint32_t _txCount;
    uint32_t _rxCount;
    uint32_t _rxFailCount;

    void sendRequest(const char* cmd, Purpose purpose);
    void handleResponseLine(const String &line);
    void handleTimeout();
    void dispatchNext();
    void sendConfigCommands(); // блокирующий вызов
    void onExchangeSuccess();
    void onExchangeFailure();

    static bool parseOneFloat(const String &line, float &out);
    static bool parseTwoFloats(const String &line, float &a, float &b);
};

#endif