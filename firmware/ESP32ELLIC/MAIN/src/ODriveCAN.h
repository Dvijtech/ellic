#pragma once

#include <Arduino.h>
#include "driver/twai.h"

struct OdriveSnapshot {
    bool online;
    int axisState;
    uint32_t axisError;
    uint32_t motorError;
    uint32_t controllerError;
    uint32_t encoderError;
    bool trajectoryDone;
    float iq;
    float busVoltage;
    float busCurrent;
    float velEstimate;
    float currentPosition;
    bool positionValid;
    uint32_t positionTimestampMs;
    uint32_t diagnosticsTimestampMs;
    uint32_t txCount;
    uint32_t rxCount;
    uint32_t rxFailCount;
    uint8_t lastTxCommand;
    uint8_t lastRxCommand;
};

class Telemetry;

class ODriveCAN {
public:
    static constexpr uint32_t CAN_BITRATE = 250000;
    static constexpr gpio_num_t CAN_TX_PIN = GPIO_NUM_16;
    static constexpr gpio_num_t CAN_RX_PIN = GPIO_NUM_17;
    static constexpr uint32_t CAN_NODE_STALE_MS = 300;

    static constexpr uint8_t RIGHT_NODE_ID = 1;
    static constexpr uint8_t LEFT_NODE_ID = 2;

    static constexpr uint8_t CMD_HEARTBEAT = 0x01;
    static constexpr uint8_t CMD_MOTOR_ERROR = 0x03;
    static constexpr uint8_t CMD_ENCODER_ERROR = 0x04;
    static constexpr uint8_t CMD_ENCODER_ESTIMATES = 0x09;
    static constexpr uint8_t CMD_SET_INPUT_POS = 0x0C;
    static constexpr uint8_t CMD_GET_IQ = 0x14;
    static constexpr uint8_t CMD_GET_BUS_VOLTAGE_CURRENT = 0x17;
    static constexpr uint8_t CMD_CONTROLLER_ERROR = 0x1D;

    explicit ODriveCAN(Telemetry* telemetry = nullptr);

    bool begin();
    void setTelemetry(Telemetry* telemetry);
    void update();

    bool moveRight(float wheelDelta);
    bool moveLeft(float wheelDelta);

    bool getRightSnapshot(OdriveSnapshot& snapshot) const;
    bool getLeftSnapshot(OdriveSnapshot& snapshot) const;

    OdriveSnapshot getSnapshotRight() const;
    OdriveSnapshot getSnapshotLeft() const;

    bool rightPositionValid() const;
    bool leftPositionValid() const;
    float rightCurrentPosition() const;
    float leftCurrentPosition() const;

    // Kept as a no-op because runtime ASCII/UART configuration is disabled by the specification.
    void updateConfigure();

private:
    struct Channel {
        uint8_t nodeId;
        bool online;
        bool positionValid;
        float currentPosition;
        float velEstimate;
        uint32_t positionTimestampMs;
        uint32_t lastValidFrameMs;
        uint32_t diagnosticsTimestampMs;
        uint32_t axisError;
        uint8_t axisState;
        uint32_t motorError;
        uint32_t encoderError;
        uint32_t controllerError;
        bool trajectoryDone;
        float iq;
        float busVoltage;
        float busCurrent;
        uint32_t txCount;
        uint32_t rxCount;
        uint32_t rxFailCount;
        uint8_t lastTxCommand;
        uint8_t lastRxCommand;
    };

    Telemetry* telemetry_;
    Channel right_;
    Channel left_;
    bool initialized_;

    static uint32_t makeCanId(uint8_t nodeId, uint8_t command);
    static bool idMatches(uint32_t id, uint8_t nodeId, uint8_t command);
    static Channel makeChannel(uint8_t nodeId);

    bool sendSetInputPos(Channel& channel, float newPosition);
    bool sendFrame(uint32_t canId, const uint8_t* data, uint8_t len, Channel& channel, uint8_t command);
    bool readFrame(twai_message_t& message);
    void processFrame(const twai_message_t& message);
    Channel* channelForNode(uint8_t nodeId);
    void markValid(Channel& channel, uint8_t command, uint32_t now);
    void updateOnlineStates(uint32_t now);
    void logOffline(Channel& channel, const char* name);
    void logOnline(Channel& channel, const char* name);
    void logRxError(const char* message);
};
