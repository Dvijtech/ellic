#pragma once
#include <Arduino.h>
#include "Config.h"

class Telemetry;

// ODriveCAN: единственный модуль, обращающийся к CAN-шине ODrive (раздел 0, 14).
// Обслуживает оба логических канала - RIGHT (node_id=1) и LEFT (node_id=2) -
// в одном экземпляре. Хранит последнюю известную позицию/скорость каждого
// WHEEL, диагностический кэш и online-статус каждого канала независимо
// (раздел 2, 13). Конфигурация самих ODrive (node_id, rate_ms и т.п.)
// выполняется заранее вне ESP32 (раздел 10.2) - соответствующего публичного
// configure() в этом классе намеренно нет.
class ODriveCAN {
public:
    ODriveCAN();

    void setTelemetry(Telemetry* telemetry);

    // Инициализирует TWAI-контроллер ESP32 (раздел 4: TX=GPIO16, RX=GPIO17,
    // 250000 бит/с, транссивер SN65HVD230).
    void begin();

    // Вызывать в каждом проходе loop(): принимает все доступные CAN-кадры,
    // обновляет диагностический кэш LEFT/RIGHT и их online-статус (раздел 14.1).
    // Не блокируется и не ждёт конкретных сообщений (раздел 14.3).
    void update();

    // MoveWheel для соответствующего колеса (раздел 9, 14.2):
    // newPosition = последняя валидная Pos_Estimate + delta -> Set Input Pos.
    // Возвращает false, если позиция WHEEL недоступна или устарела
    // (раздел 13.1) - в этом случае команда для этого колеса в этом цикле
    // не отправляется. Второе колесо на это не влияет.
    bool moveLeftWheel(float delta);
    bool moveRightWheel(float delta);

    OdriveSnapshot getLeftSnapshot() const;
    OdriveSnapshot getRightSnapshot() const;

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
    };

    static void initChannel(WheelChannel &ch, uint8_t nodeId, const char* label);
    WheelChannel* channelForNode(uint8_t nodeId);

    // frameData должен указывать минимум на dlc байт полезной нагрузки кадра.
    void processFrame(uint32_t canId, const uint8_t* frameData, uint8_t dlc, uint32_t nowMs);
    void refreshOnlineState(uint32_t nowMs);

    bool moveWheelInternal(WheelChannel &ch, float delta);
    OdriveSnapshot snapshotFrom(const WheelChannel &ch) const;

    Telemetry* _telemetry;
    WheelChannel _right; // node_id = RIGHT_ODRIVE_NODE_ID (1)
    WheelChannel _left;  // node_id = LEFT_ODRIVE_NODE_ID  (2)
};
