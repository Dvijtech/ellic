#include <Arduino.h>
#include <Wire.h>
#include <AS5600.h>

AS5600 encoder;

// =========================
// Настройки
// =========================
constexpr uint32_t READ_INTERVAL_MS  = 50;    // 20 Гц
constexpr uint32_t PRINT_INTERVAL_MS = 500;   // 2 Гц
constexpr uint32_t STOP_TIMEOUT_MS   = 400;   // 300-500 мс

constexpr float EMA_ALPHA = 0.35f;

constexpr float STOP_DELTA_DEG   = 1.0f;
constexpr float RESTART_DELTA_DEG = 7.0f;

// =========================

enum MotionState
{
    MOVING,
    STOP
};

MotionState state = MOVING;

// Таймеры
uint32_t lastReadTime = 0;
uint32_t lastPrintTime = 0;

// Сырые данные
uint16_t rawAngle = 0;

// Последнее значение AS5600
int32_t previousRaw = 0;
bool firstRead = true;

// Непрерывный угол
float continuousAngle = 0.0f;

// EMA
float filteredAngle = 0.0f;

// Зафиксированный угол
float lockedAngle = 0.0f;

// Последнее движение
float previousFiltered = 0.0f;
uint32_t lastMovementTime = 0;

//==================================================

void readEncoder();
void updateContinuousAngle();
void filterAngle();
void detectMovement();
void printStatus();
void sendPositionToODrive(float angle);

//==================================================

void setup()
{
    Serial.begin(115200);
    delay(500);

    Wire.begin();
    Wire.setClock(100000);

    Serial.println();
    Serial.println("Initializing AS5600...");

    if (!encoder.begin())
    {
        Serial.println("ERROR: AS5600 not found.");
        while (true);
    }

    Serial.println("AS5600 OK");

    /// if (!encoder.detectMagnet())
    if (!encoder.magnetDetected())
    {
        Serial.println("WARNING: Magnet not detected.");
    }
    else
    {
        Serial.println("Magnet detected.");
    }

    lastMovementTime = millis();
}

//==================================================

void loop()
{
    uint16_t raw = encoder.rawAngle();
    uint16_t ang = encoder.readAngle();

    Serial.print("RAW=");
    Serial.print(raw);

    Serial.print("  LOW4=");
    Serial.print(raw & 0x0F);

    Serial.print("  ANG=");
    Serial.println(ang);

    delay(5);
}
//==================================================

void sendPositionToODrive(float angle)
{
    // Заглушка.
    // Здесь позже будет UART-команда в ODrive.
}