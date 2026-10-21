#pragma once
#include <Arduino.h>
#include "Config.h"

class Telemetry;

// ODriveCAN: единственный модуль, обращающийся к CAN-шине ODrive (раздел 0, 12).
// Обслуживает оба логических канала - RIGHT (node_id=1) и LEFT (node_id=2) -
// в одном экземпляре. Хранит последнюю известную позицию/скорость каждого
// WHEEL, диагностический кэш и online-статус каждого канала независимо.
// Конфигурация самих ODrive (node_id, rate_ms и т.п.) выполняется заранее вне
// ESP32 (раздел 10) - публичного configure() в этом классе намеренно нет.
class ODriveCAN {
public:
    ODriveCAN();

    void setTelemetry(Telemetry* telemetry);

    // Инициализирует TWAI-контроллер ESP32 (раздел 4: TX=GPIO16, RX=GPIO17,
    // 250000 бит/с, трансивер SN65HVD230, очередь приёма TWAI_RX_QUEUE_LEN).
    void begin();

    // Вызывать в каждом проходе loop(): принимает все доступные CAN-кадры,
    // обновляет диагностический кэш LEFT/RIGHT и online-статус, раз в
    // CAN_STATUS_POLL_MS опрашивает состояние контроллера TWAI и при bus-off
    // запускает восстановление (раздел 12.3). Не блокируется (раздел 12.5).
    void update();

    // MoveWheel для соответствующего колеса (раздел 9):
    // newPosition = последняя валидная Pos_Estimate + delta -> Set Input Pos.
    // Возвращает false, если позиция WHEEL недоступна или устарела - в этом
    // случае команда для этого колеса в этом цикле не отправляется. Второе
    // колесо на это не влияет.
    bool moveLeftWheel(float delta);
    bool moveRightWheel(float delta);

    OdriveSnapshot getLeftSnapshot() const;
    OdriveSnapshot getRightSnapshot() const;
    CanBusSnapshot getBusSnapshot() const;

    // Сбрасывает статистику за период (posAgeMaxMs). Вызывает Telemetry после
    // выдачи пакета.
    void resetPeriodStats();

private:
    struct WheelChannel {
        uint8_t nodeId;
        const char* label;

        bool online;
        bool everReceivedFrame;
        uint32_t lastFrameMs;

        bool hasValidPosition;
        uint32_t lastPosEstimateMs;
        float posEstimate;
        float velEstimate;

        int      axisState;
        uint32_t axisError;
        uint64_t motorError;
        uint32_t encoderError;
        uint32_t controllerError;
        bool     trajectoryDone;

        float Iq;
        float busVoltage;
        float busCurrent;

        uint32_t txCount;
        uint32_t rxCount;
        uint32_t rxFailCount;
        uint32_t diagnosticsTimestampMs;

        uint32_t txFailCount;
        uint32_t offlineCount;
        uint32_t axisErrCount;
        uint32_t motorErrCount;
        uint32_t encoderErrCount;
        uint32_t controllerErrCount;

        uint32_t cmdCount[ODRIVE_CMD_SLOTS];
        uint32_t posAgeMaxMs;
    };

    static void initChannel(WheelChannel &ch, uint8_t nodeId, const char* label);
    WheelChannel* channelForNode(uint8_t nodeId);

    // frameData должен указывать минимум на dlc байт полезной нагрузки кадра.
    void processFrame(uint32_t canId, const uint8_t* frameData, uint8_t dlc, uint32_t nowMs);
    void refreshOnlineState(uint32_t nowMs);
    void pollBusStatus(uint32_t nowMs);

    bool moveWheelInternal(WheelChannel &ch, float delta);
    OdriveSnapshot snapshotFrom(const WheelChannel &ch) const;

    Telemetry* _telemetry;
    WheelChannel _right; // node_id = RIGHT_ODRIVE_NODE_ID (1)
    WheelChannel _left;  // node_id = LEFT_ODRIVE_NODE_ID  (2)

    bool     _driverInstalled;
    bool     _recoveryRequested;
    uint32_t _lastBusPollMs;
    CanBusSnapshot _bus;
};
