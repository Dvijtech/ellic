#pragma once
#include <Arduino.h>

// Версия прошивки (печатается при старте в Serial строкой "# ...").
#define FW_VERSION "v3.1"

// =======================================================================
// Аппаратные подключения (раздел 4 спецификации)
// =======================================================================

// AS5600 (I2C)
#define AS5600_SDA_PIN 21
#define AS5600_SCL_PIN 22
#define AS5600_I2C_CLOCK_HZ 50000

// AS5600 регистр RAW ANGLE (12 бит, 0x0C/0x0D)
#define AS5600_I2C_ADDR 0x36
#define AS5600_RAW_ANGLE_REG 0x0C

// Тормоза
#define LEFT_BRAKE_PIN 32  // второй провод на землю
#define RIGHT_BRAKE_PIN 33 // второй провод на землю
// Тормоз нажат при digitalRead(pin) == LOW (см. раздел 4)

// Кнопка стрельбы (INPUT_PULLUP, нажата при LOW). GPIO23: свободен (не занят
// AS5600, тормозами, CAN, не strapping-пин), 18 зарезервирован под
// EllicVRButton, 27 - под джойстик, 25/26 - прежние UART-линии ODrive.
// Подключение (раздел 4 спецификации): синий провод Nerf -> GPIO23,
// фиолетовый провод Nerf -> GND ESP32, кнопка без фиксации между GPIO23 и GND.
// Минус батареек Nerf с землёй ESP32/ELLIC НЕ соединяется.
#define FIRE_BUTTON_PIN 23

// CAN (ESP32 TWAI, трансивер SN65HVD230)
#define CAN_TX_PIN 16
#define CAN_RX_PIN 17
static const uint32_t CAN_BITRATE = 250000; // бит/с

// =======================================================================
// Константы (раздел 5 спецификации)
// =======================================================================

static const uint32_t CONTROL_PERIOD_MS      = 300;    // период MotionController.update()
static const float    MOTOR_GEAR_RATIO       = 6.33f;  // передаточное число редуктора
static const float    TURN_ZONE_DEG          = 10.0f;  // ширина зоны поворота
static const float    TURN_STEP              = 0.7f;   // шаг противоположного колеса при повороте (оборотов мотора)

static const float    ENCODER_SIGN           = -1.0f;  // инверсия направления энкодера (только для NORMAL)

static const float    LEFT_WHEEL_SIGN        = 1.0f;
static const float    RIGHT_WHEEL_SIGN       = -1.0f;

// Период опроса AS5600. Раньше читали на каждом проходе loop() (тысячи раз
// в секунду). Вал на 10 мс проходит единицы градусов, до порога 180° далеко.
static const uint32_t ENCODER_PERIOD_MS      = 10;

// Период сбора и печати пакета M/L/R/E/D/C. 200 мс (5 Гц) - как в исходном
// ТЗ телеметрии; при 100 мс BLE не успевал отправлять (теряло ~40% строк).
static const uint32_t TELEMETRY_PERIOD_MS    = 200;

// 1 - дублировать телеметрию в Serial, 0 - только BLE.
#define TELEMETRY_TO_SERIAL 1

// Скорость Serial. ВАЖНО: в platformio.ini должен стоять тот же
// monitor_speed = 921600 (и такая же скорость в любой терминальной программе).
// При 115200 печать пакета (~500 байт) блокировала бы loop() на ~45 мс.
static const uint32_t SERIAL_BAUD            = 921600;

// Антидребезг кнопки стрельбы: новое состояние принимается, когда сигнал
// не менялся не менее этого времени. Если на длинном проводе появятся ложные
// срабатывания, сначала можно увеличить это значение (30-50 мс).
static const uint32_t FIRE_DEBOUNCE_MS       = 15;

// 0 (по умолчанию) - fire / fire_count уходят только в периодическом пакете
// (запись M). 1 - дополнительно, при каждом изменении состояния кнопки, сразу
// выдаётся одна строка M вне расписания. Для разбора потока безопаснее 0:
// выстрел определяется по росту fire_count.
#define FIRE_SEND_IMMEDIATE 0

static const uint32_t CAN_NODE_STALE_MS      = 300;    // порог offline / "протухшей" позиции

// TWAI: очередь приёма была 5 кадров по умолчанию - при кратковременной
// задержке loop() кадры двух ODrive (14 кадров пачкой) терялись.
static const uint32_t TWAI_RX_QUEUE_LEN      = 64;
static const uint32_t TWAI_TX_QUEUE_LEN      = 16;
static const uint32_t CAN_STATUS_POLL_MS     = 50;     // опрос состояния TWAI (bus-off и счётчики)

// CAN node_id (раздел 4, 10.3)
static const uint8_t RIGHT_ODRIVE_NODE_ID = 1;
static const uint8_t LEFT_ODRIVE_NODE_ID  = 2;

// Число отслеживаемых типов циклических CAN-кадров ODrive (по порядку):
// 0 Heartbeat(0x01), 1 MotorError(0x03), 2 EncoderError(0x04),
// 3 EncoderEstimates(0x09), 4 Iq(0x14), 5 BusVI(0x17), 6 ControllerError(0x1D)
static const uint8_t ODRIVE_CMD_SLOTS = 7;

// =======================================================================
// Общие перечисления и структуры-снапшоты (раздел 13.4)
// =======================================================================

enum class MotionMode { NORMAL, CALM, TURN };

struct EncoderSnapshot {
    float rawAngle;        // последнее валидное значение AS5600, 0..360
    float continuousAngle; // накопленный (безразрывный) угол
    float lastDelta;       // последняя дельта, посчитанная в Encoder::update()
    uint32_t readErrors;   // накопительно: неудачные чтения AS5600
    uint32_t readMaxUs;    // макс. длительность чтения AS5600 за период телеметрии, мкс
};

struct FireSnapshot {
    bool     pressed;      // дебаунснутое состояние: true - кнопка нажата
    uint32_t pressCount;   // число нажатий (по фронту) с запуска ESP
};

struct MotionSnapshot {
    float valDelta;
    float leftWheelDelta;
    float rightWheelDelta;
    MotionMode mode;
    bool leftBrake;
    bool rightBrake;
    bool inTurnZone;
};

struct OdriveSnapshot {
    bool online;

    int      axisState;
    uint32_t axisError;
    uint64_t motorError;       // Get Motor Error (0x03) - 64 бита у fw 0.5.6
    uint32_t encoderError;     // Get Encoder Error (0x04)
    uint32_t controllerError;  // Get Controller Error (0x1D)
    bool     trajectoryDone;   // Heartbeat.Trajectory_Done_Flag

    float Iq;           // Get Iq (0x14), Iq_Measured
    float velEstimate;  // Get Encoder Estimates (0x09), Vel_Estimate
    float busVoltage;   // Get Bus Voltage Current (0x17)
    float busCurrent;   // Get Bus Voltage Current (0x17)

    uint32_t txCount;
    uint32_t rxCount;
    uint32_t rxFailCount;       // команды, пропущенные из-за недоступной/устаревшей позиции
    uint32_t diagnosticsTimestampMs;

    uint32_t txFailCount;       // отказы twai_transmit()
    uint32_t offlineCount;      // переходы online -> offline
    uint32_t axisErrCount;      // новые ненулевые значения axisError
    uint32_t motorErrCount;
    uint32_t encoderErrCount;
    uint32_t controllerErrCount;

    uint32_t cmdCount[ODRIVE_CMD_SLOTS]; // накопительно: принятые кадры по типам
    uint32_t posAgeMaxMs;       // макс. возраст позиции в момент команды за период, мс
};

// Состояние контроллера TWAI (накопительные счётчики драйвера).
struct CanBusSnapshot {
    uint8_t  state;             // 0 STOPPED, 1 RUNNING, 2 BUS_OFF, 3 RECOVERING
    uint32_t txErrCounter;      // TEC (текущее значение)
    uint32_t rxErrCounter;      // REC (текущее значение)
    uint32_t rxMissed;          // кадры, потерянные из-за переполнения очереди приёма
    uint32_t arbLost;
    uint32_t busError;
    uint32_t txFailedHw;        // отправка не удалась на уровне контроллера
    uint32_t busOffRecoveries;  // сколько раз запускалось восстановление после bus-off
};

struct TelemetrySample {
    EncoderSnapshot encoder;
    MotionSnapshot  motion;
    OdriveSnapshot  left;
    OdriveSnapshot  right;
};
