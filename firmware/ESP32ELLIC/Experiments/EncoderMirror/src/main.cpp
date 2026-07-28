#include <Arduino.h>
#include <Wire.h>
#include <AS5600.h>
#include "BLETrigger.h"

// ====================================================
// DEBUG FLAGS — меняйте 0→1 чтобы включить проверки
// ====================================================
#define DEBUG_DUMP_CSV      1   // табличный вывод состояния
#define DEBUG_ASSERTS       1   // проверка инвариантов
#define DEBUG_LOOP_TIMING   1   // замер времени loop()
#define DEBUG_ENCODER_TEST  0   // тест энкодера в setup() (10 сек)

//====================================================
//                      AS5600
//====================================================

AS5600 encoder;


//====================================================
//                      ODrive
//====================================================

HardwareSerial &LeftODrive  = Serial1;
HardwareSerial &RightODrive = Serial2;


//====================================================
//                   PIN CONFIG
//====================================================

constexpr int SDA_PIN = 21;
constexpr int SCL_PIN = 22;

constexpr int LEFT_RX  = 16;
constexpr int LEFT_TX  = 17;

constexpr int RIGHT_RX = 26;
constexpr int RIGHT_TX = 25;


// тормозные ручки

constexpr int LEFT_BRAKE_PIN  = 32;
constexpr int RIGHT_BRAKE_PIN = 33;


//====================================================
//                   SETTINGS
//====================================================

constexpr float GEAR_RATIO = 4.4f;

constexpr float TURN_STEP = 0.03f;


constexpr uint32_t CONTROL_PERIOD = 50;
constexpr uint32_t PRINT_PERIOD = 100;


//====================================================
//                   VARIABLES
//====================================================

// ---------- Вал (Val) ----------
float rawAngleV = 0;
float lastAngleV = 0;
float deltaAngleV = 0;
float continuousAngleV = 0;
float targetTurnsV = 0;


// временный поворотный offset

// ---------- Колеса (Wheel) ----------
float leftTurnOffsetW = 0;
float rightTurnOffsetW = 0;


// сохраненный offset после поворота

float leftHoldOffsetW = 0;
float rightHoldOffsetW = 0;

// Последние отправленные команды колесам
float leftTargetW = 0;
float rightTargetW = 0;

// состояние поворота

bool turning = false;


// состояние ODrive

bool leftClosedLoop = false;
bool rightClosedLoop = false;



unsigned long lastControl = 0;
unsigned long lastPrint = 0;


//===========добавил переменные для проверки живости ODrive=========== 
bool leftODriveAlive = false;
bool rightODriveAlive = false;

unsigned long leftLastSeen = 0;
unsigned long rightLastSeen = 0;

constexpr uint32_t ODRIVE_CHECK_PERIOD = 1000;
constexpr uint32_t ODRIVE_BOOT_DELAY = 1500;

unsigned long lastODriveCheck = 0;


void sendPosition(HardwareSerial &port, float turns);
void setClosedLoop(HardwareSerial &port);

//======== функция проверки живости ODrive, возвращает true если ODrive отвечает на команду ========

bool checkODrive(HardwareSerial &port)
{
    while (port.available())
        port.read();

    port.println("r vbus_voltage");

    unsigned long start = millis();

    while (millis() - start < 100)
    {
        if (port.available())
        {
            String s = port.readStringUntil('\n');
            s.trim();

            if (s.length() > 0)
                return true;
        }
    }

    return false;
}
//=================добавил функцию для переинициализации ODrive, если он не отвечает========================

void reinitLeftODrive()
{
    Serial.println("LEFT ODrive REINIT");

    leftClosedLoop = false;

    delay(ODRIVE_BOOT_DELAY);

    float pos = targetTurnsV + leftHoldOffsetW;

    sendPosition(LeftODrive, pos);
    delay(50);

    setClosedLoop(LeftODrive);
    delay(100);

    leftClosedLoop = true;

    Serial.println("LEFT ODrive READY");
}


void reinitRightODrive()
{
    Serial.println("RIGHT ODrive REINIT");

    rightClosedLoop = false;

    delay(ODRIVE_BOOT_DELAY);

    float pos = -targetTurnsV + rightHoldOffsetW;

    sendPosition(RightODrive, pos);
    delay(50);

    setClosedLoop(RightODrive);
    delay(100);

    rightClosedLoop = true;

    Serial.println("RIGHT ODrive READY");
}

//====


float readAngle()
{
    return encoder.readAngle() * 360.0f / 4096.0f;
}


//====================================================

float shortestDelta(float now, float old)
{
    float d = now - old;

    if(d > 180)
        d -= 360;

    if(d < -180)
        d += 360;

    return d;
}


//====================================================

bool inTurnZone(float angle)
{
    if(angle >= 350)
        return true;

    if(angle <= 10)
        return true;

    if(angle >= 170 && angle <= 190)
        return true;

    return false;
}


//====================================================

void sendPosition(
    HardwareSerial &port,
    float turns
)
{
    char buffer[64];

    snprintf(
        buffer,
        sizeof(buffer),
        "w axis0.controller.input_pos %.4f",
        turns
    );

    port.println(buffer);
}


//====================================================

void setIdle(
    HardwareSerial &port
)
{
    port.println(
        "w axis0.requested_state 1"
    );
}


//====================================================

void setClosedLoop(
    HardwareSerial &port
)
{
    port.println(
        "w axis0.requested_state 8"
    );
}


//====================================================

void enableLeft()
{
    if(!leftClosedLoop)
    {
        setClosedLoop(LeftODrive);
        leftClosedLoop = true;
    }
}


//====================================================

void enableRight()
{
    if(!rightClosedLoop)
    {
        setClosedLoop(RightODrive);
        rightClosedLoop = true;
    }
}


//====================================================

void disableLeft()
{
    if(leftClosedLoop)
    {
        setIdle(LeftODrive);
        leftClosedLoop = false;
    }
}


//====================================================

void disableRight()
{
    if(rightClosedLoop)
    {
        setIdle(RightODrive);
        rightClosedLoop = false;
    }
}


//====================================================
// фиксация текущего положения после поворота
//====================================================

void finishTurn()
{

    leftHoldOffsetW += leftTurnOffsetW;
    rightHoldOffsetW += rightTurnOffsetW;


    leftTurnOffsetW = 0;
    rightTurnOffsetW = 0;


    turning = false;
}

// ====================================================
// DEBUG HELPERS
// ====================================================

enum class State {
    NORMAL = 0,
    BOTH_BRAKE = 1,
    LEFT_TURN = 2,
    RIGHT_TURN = 3,
    OUT_ZONE = 4
};

const char* stateName(State st) {
    switch(st) {
        case State::NORMAL:     return "NORMAL";
        case State::BOTH_BRAKE: return "BOTH_BRAKE";
        case State::LEFT_TURN:  return "LEFT_TURN";
        case State::RIGHT_TURN: return "RIGHT_TURN";
        case State::OUT_ZONE:   return "OUT_ZONE";
    }
    return "UNKNOWN";
}

#if DEBUG_DUMP_CSV
void dumpState(State st, unsigned long now,
               bool leftBrake, bool rightBrake, bool zone)
{
    // CSV формат для Serial Plotter / Excel
    Serial.print(now);                Serial.print(',');
    Serial.print(rawAngleV, 2);       Serial.print(',');
    Serial.print(continuousAngleV,2); Serial.print(',');
    Serial.print(targetTurnsV, 4);    Serial.print(',');
    Serial.print(leftBrake);          Serial.print(',');
    Serial.print(rightBrake);         Serial.print(',');
    Serial.print(zone);               Serial.print(',');
    Serial.print((int)st);            Serial.print(',');
    Serial.print(leftTurnOffsetW,4);  Serial.print(',');
    Serial.print(rightTurnOffsetW,4); Serial.print(',');
    Serial.print(leftHoldOffsetW,4);  Serial.print(',');
    Serial.print(rightHoldOffsetW,4); Serial.print(',');
    Serial.print(leftClosedLoop);     Serial.print(',');
    Serial.print(rightClosedLoop);    Serial.print(',');
    Serial.print(leftODriveAlive);    Serial.print(',');
    Serial.print(rightODriveAlive);   Serial.print(',');
    Serial.print(leftTargetW, 4);     Serial.print(',');
    Serial.println(rightTargetW, 4);
}
#endif

#if DEBUG_ASSERTS
void checkInvariants(bool leftBrake, bool rightBrake)
{
    // Проверка 1: deltaAngleV не должен скачивать на >90° за один цикл
    if (fabs(deltaAngleV) > 90.0f) {
        Serial.println("!!! ALERT: deltaAngleV > 90 — возможно, lastAngleV не успел обновиться или энкодер глючит");
    }

    // Проверка 2: offset'ы не должны накапливаться бесконечно
    if (fabs(leftHoldOffsetW) > 100.0f || fabs(rightHoldOffsetW) > 100.0f) {
        Serial.println("!!! ALERT: holdOffset разошелся — возможно, finishTurn() не вызывается");
    }

    // Проверка 3: если turning==true, но оба тормоза отпущены — странно
    if (turning && !leftBrake && !rightBrake) {
        Serial.println("!!! ALERT: turning=true без тормозов — finishTurn() не сработал?");
    }

    // Проверка 4: если в зоне поворота и нажат тормоз, но turning=false — возможно, зона не сработала
    if ((leftBrake || rightBrake) && inTurnZone(rawAngleV) && !turning && !(leftBrake && rightBrake)) {
        // один тормоз нажат, в зоне, но turning еще не установлен — нормально для первого цикла
        // но если повторяется — странно
        static unsigned long lastAlert = 0;
        if (millis() - lastAlert > 500) {
            Serial.println("!!! WARN: тормоз + зона, но turning=false (может быть нормально на 1-м цикле)");
            lastAlert = millis();
        }
    }
}
#endif

//====================================================

void setup()
{

    Serial.begin(115200);


    Serial.println();
    Serial.println("==============================");
    Serial.println("ELLIC TURN CONTROL (DEBUG)");
    Serial.println("==============================");

#if DEBUG_DUMP_CSV
    Serial.println("time,rawAngle,contAngle,targetTurns,leftBrake,rightBrake,zone,state,leftTurnOff,rightTurnOff,leftHoldOff,rightHoldOff,leftCL,rightCL,leftAlive,rightAlive,leftTarget,rightTarget");
#endif

    Wire.begin(
        SDA_PIN,
        SCL_PIN
    );

    Wire.setClock(50000);


    delay(300);



    if(!encoder.begin())
    {
        Serial.println("AS5600 ERROR");

        while(true)
            delay(1000);
    }


    if(!encoder.magnetDetected())
    {
        Serial.println("MAGNET ERROR");

        while(true)
            delay(1000);
    }


    Serial.println("AS5600 OK");



    pinMode(
        LEFT_BRAKE_PIN,
        INPUT_PULLDOWN
    );


    pinMode(
        RIGHT_BRAKE_PIN,
        INPUT_PULLDOWN
    );

    // Отладка: показать сырые значения тормозов
    Serial.print("LEFT_BRAKE raw=");  Serial.println(digitalRead(LEFT_BRAKE_PIN));
    Serial.print("RIGHT_BRAKE raw="); Serial.println(digitalRead(RIGHT_BRAKE_PIN));
    Serial.println("(0 = отпущен, 1 = нажат — при INPUT_PULLDOWN)");


    LeftODrive.begin(
        115200,
        SERIAL_8N1,
        LEFT_RX,
        LEFT_TX
    );


    RightODrive.begin(
        115200,
        SERIAL_8N1,
        RIGHT_RX,
        RIGHT_TX
    );


    delay(1000);



    rawAngleV = readAngle();
    lastAngleV = rawAngleV;

    /// delay(5000);

    enableLeft();
    enableRight();

    BLETrigger_begin();

    Serial.println("READY");

#if DEBUG_ENCODER_TEST
    Serial.println("=== ENCODER TEST ===");
    Serial.println("Крутите руль вручную. continuousAngleV должен расти монотонно.");
    float testCont = 0;
    float testLast = lastAngleV;
    for (int i = 0; i < 200; i++) {  // 10 секунд при 50 мс
        float a = readAngle();
        float d = shortestDelta(a, testLast);
        testCont += d;
        testLast = a;
        Serial.print("RAW:"); Serial.print(a,2);
        Serial.print(" DELTA:"); Serial.print(d,2);
        Serial.print(" CONT:"); Serial.println(testCont,2);
        delay(50);
    }
    Serial.println("=== TEST END ===");
    // Восстановить lastAngleV после теста
    lastAngleV = readAngle();
    continuousAngleV = 0;
#endif
}


//====================================================

void loop()
{
#if DEBUG_LOOP_TIMING
    unsigned long loopStart = millis();
#endif

    BLETrigger_update();

    if (millis() - lastODriveCheck >= ODRIVE_CHECK_PERIOD)
    {
        lastODriveCheck = millis();

        bool leftNow = checkODrive(LeftODrive);
        bool rightNow = checkODrive(RightODrive);

        if (leftNow && !leftODriveAlive)
        {
            Serial.println("LEFT ODrive POWER RESTORED");
            reinitLeftODrive();
        }

        if (rightNow && !rightODriveAlive)
        {
            Serial.println("RIGHT ODrive POWER RESTORED");
            reinitRightODrive();
        }

        if (!leftNow)
        {
            leftClosedLoop = false;
        }

        if (!rightNow)
        {
            rightClosedLoop = false;
        }

        leftODriveAlive = leftNow;
        rightODriveAlive = rightNow;
    }

    rawAngleV = readAngle();



    deltaAngleV = shortestDelta(
        rawAngleV,
        lastAngleV
    );


    continuousAngleV += deltaAngleV;



    targetTurnsV =
        continuousAngleV *
        GEAR_RATIO /
        360.0f;



    bool leftBrake =
        digitalRead(
            LEFT_BRAKE_PIN
        );


    bool rightBrake =
        digitalRead(
            RIGHT_BRAKE_PIN
        );


    bool zone =
        inTurnZone(rawAngleV);


    State currentState = State::NORMAL;


    if(millis() - lastControl >= CONTROL_PERIOD)
    {

        lastControl = millis();



        //--------------------------------------------
        // оба тормоза
        //--------------------------------------------

        if(leftBrake && rightBrake)
        {
            currentState = State::BOTH_BRAKE;

            disableLeft();
            disableRight();

            turning = true;

        }



        //--------------------------------------------
        // левый тормоз
        //--------------------------------------------

        else if(leftBrake)
        {

            if(zone)
            {
                currentState = State::LEFT_TURN;

                turning = true;


                disableLeft();
                enableRight();



                rightTurnOffsetW += TURN_STEP;



                rightTargetW =
                -targetTurnsV
                + rightHoldOffsetW
                + rightTurnOffsetW;

                sendPosition(RightODrive, rightTargetW);

            }
            else
            {
                currentState = State::OUT_ZONE;

                disableLeft();
                disableRight();

            }

        }



        //--------------------------------------------
        // правый тормоз
        //--------------------------------------------

        else if(rightBrake)
        {

            if(zone)
            {
                currentState = State::RIGHT_TURN;

                turning = true;


                disableRight();
                enableLeft();



                leftTurnOffsetW += TURN_STEP;



                leftTargetW =
                + targetTurnsV
                + leftHoldOffsetW
                + leftTurnOffsetW;

                sendPosition(LeftODrive, leftTargetW);

            }
            else
            {
                currentState = State::OUT_ZONE;

                disableLeft();
                disableRight();

            }

        }



        //--------------------------------------------
        // обычный режим
        //--------------------------------------------

        else
        {
            currentState = State::NORMAL;

            if(turning)
            {
                finishTurn();
            }



            enableLeft();
            enableRight();



            leftTargetW = targetTurnsV + leftHoldOffsetW;
            sendPosition(LeftODrive, leftTargetW);


            rightTargetW = -targetTurnsV + rightHoldOffsetW;
            sendPosition(RightODrive, rightTargetW);

        }

    }




    if(millis() - lastPrint >= PRINT_PERIOD)
    {

        lastPrint = millis();

#if DEBUG_DUMP_CSV
        dumpState(currentState, millis(), leftBrake, rightBrake, zone);
#else
        Serial.print("RAW:");
        Serial.print(rawAngleV,2);


        Serial.print(" TARGET:");
        Serial.print(targetTurnsV,3);


        Serial.print(" LB:");
        Serial.print(leftBrake);


        Serial.print(" RB:");
        Serial.print(rightBrake);


        Serial.print(" Z:");
        Serial.print(zone);


        Serial.print(" LO:");
        Serial.print(leftHoldOffsetW,3);


        Serial.print(" RO:");
        Serial.println(rightHoldOffsetW,3);
#endif

    }

#if DEBUG_ASSERTS
    checkInvariants(leftBrake, rightBrake);
#endif

    lastAngleV = rawAngleV;

#if DEBUG_LOOP_TIMING
    unsigned long loopDuration = millis() - loopStart;
    if (loopDuration > CONTROL_PERIOD + 10) {
        Serial.print("!!! SLOW LOOP: ");
        Serial.print(loopDuration);
        Serial.println(" ms");
    }
#endif
}