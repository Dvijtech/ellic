#include "MotionController.h"

// ============================================================
// CONSTRUCTOR
// ============================================================

MotionController::MotionController(
    Encoder &encoder,
    ODriveUART &leftDrive,
    ODriveUART &rightDrive,
    PhaseDetector &phase,
    float gearRatio,
    float turnStep,
    uint32_t controlPeriodMs)

    : _encoder(encoder),
      _left(leftDrive),
      _right(rightDrive),
      _phase(phase),
      _gearRatio(gearRatio),
      _turnStep(turnStep),
      _controlPeriodMs(controlPeriodMs)
{
}

// ============================================================
// BEGIN
// ============================================================

void MotionController::begin()
{
    _previousValAngle =
        _encoder.valContinuousAngle();

    _valBaseAngle =
        _previousValAngle;

    _leftBaseTarget = 0.0f;
    _rightBaseTarget = 0.0f;

    _leftTarget = 0.0f;
    _rightTarget = 0.0f;

    _leftTurnOffset = 0.0f;
    _rightTurnOffset = 0.0f;

    _valSpeedDegS = 0.0f;

    _walking = false;
    _stopped = true;
    _turning = false;

    _lastControl = millis();
    _lastValSampleTime = millis();

    // ODrive остаётся в Closed Loop.
    //
    // disable()/enable() не используются
    // при обычной ходьбе и поворотах.

    _left.enable();
    _right.enable();
}

// ============================================================
// READ BRAKES
// ============================================================

void MotionController::readBrakes(
    bool &leftBrake,
    bool &rightBrake) const
{
    leftBrake =
        digitalRead(LEFT_BRAKE_PIN);

    rightBrake =
        digitalRead(RIGHT_BRAKE_PIN);
}

// ============================================================
// UPDATE VAL SPEED
// ============================================================

void MotionController::updateValSpeed()
{
    uint32_t now = millis();

    uint32_t dt =
        now - _lastValSampleTime;

    if (dt == 0)
        return;

    float currentAngle =
        _encoder.valContinuousAngle();

    float delta =
        currentAngle - _previousValAngle;

    _valSpeedDegS =
        delta * 1000.0f / (float)dt;

    _previousValAngle =
        currentAngle;

    _lastValSampleTime =
        now;
}

// ============================================================
// UPDATE STATE
// ============================================================

void MotionController::updateState()
{
    bool wasStopped =
        _stopped;

    float speed =
        fabsf(_valSpeedDegS);

    if (speed <= VAL_STOP_SPEED_DEG_S)
    {
        _stopped = true;
        _walking = false;
    }
    else
    {
        _stopped = false;
        _walking = true;
    }

    // --------------------------------------------------------
    // Переход:
    //
    // WALKING → STOPPED
    //
    // Заканчивается цикл движения вала.
    // --------------------------------------------------------

    if (!wasStopped && _stopped)
    {
        createNewBase();
    }
}

// ============================================================
// CREATE NEW BASE
// ============================================================

void MotionController::createNewBase()
{
    // --------------------------------------------------------
    // Запоминаем текущее командное положение колёс.
    //
    // Никаких команд ODrive здесь НЕ отправляем.
    // Поэтому эта операция сама по себе
    // физического движения не вызывает.
    // --------------------------------------------------------

    _leftBaseTarget =
        _leftTarget;

    _rightBaseTarget =
        _rightTarget;

    _valBaseAngle =
        _encoder.valContinuousAngle();

    resetTurnOffsets();
}

// ============================================================
// RESET TURN OFFSETS
// ============================================================

void MotionController::resetTurnOffsets()
{
    _leftTurnOffset = 0.0f;
    _rightTurnOffset = 0.0f;
}

// ============================================================
// WALKING
// ============================================================

void MotionController::updateWalking()
{
    float relativeValAngle =
        _encoder.valContinuousAngle() -
        _valBaseAngle;

    // --------------------------------------------------------
    // Один оборот коленвала = один оборот колеса.
    //
    // Но мотор-колесо имеет редуктор 4.4:1.
    //
    // Поэтому:
    //
    // 360° вала
    //      ↓
    // 360° колеса
    //      ↓
    // 4.4 оборота мотора
    // --------------------------------------------------------

    float motorTurns =
        relativeValAngle *
        _gearRatio /
        360.0f;

    // --------------------------------------------------------
    // Колёса установлены зеркально.
    //
    // Поэтому направление правого мотора
    // противоположно левому.
    // --------------------------------------------------------

    _leftTarget =
        _leftBaseTarget +
        motorTurns +
        _leftTurnOffset;

    _rightTarget =
        _rightBaseTarget -
        motorTurns +
        _rightTurnOffset;

    // --------------------------------------------------------
    // Отправляем текущую команду.
    // --------------------------------------------------------

    _left.sendPosition(
        _leftTarget);

    _right.sendPosition(
        _rightTarget);
}

// ============================================================
// TURNING
// ============================================================

void MotionController::updateTurning()
{
    bool leftBrake;
    bool rightBrake;

    readBrakes(
        leftBrake,
        rightBrake);

    // --------------------------------------------------------
    // Оба тормоза
    // --------------------------------------------------------

    if (leftBrake && rightBrake)
    {
        createNewBase();

        _turning = false;

        return;
    }

    // --------------------------------------------------------
    // Проверяем фазу коленвала.
    // --------------------------------------------------------

    bool zone =
        _phase.inTurnZone(
            _encoder.valRawAngle());

    if (!zone)  
    {
        _turning = false;

        return;
    }

    // --------------------------------------------------------
    // Левый тормоз
    //
    // Левое колесо удерживается.
    // Правое колесо получает дополнительное движение.
    // --------------------------------------------------------

    if (leftBrake && !rightBrake)
    {
        _turning = true;

        _rightTurnOffset -=
            _turnStep;

        _rightTarget =
            _rightBaseTarget +
            _rightTurnOffset;

        _right.sendPosition(
            _rightTarget);

        return;
    }

    // --------------------------------------------------------
    // Правый тормоз
    //
    // Правое колесо удерживается.
    // Левое колесо получает дополнительное движение.
    // --------------------------------------------------------

    if (rightBrake && !leftBrake)
    {
        _turning = true;

        _leftTurnOffset +=
            _turnStep;

        _leftTarget =
            _leftBaseTarget +
            _leftTurnOffset;

        _left.sendPosition(
            _leftTarget);

        return;
    }

    _turning = false;
}

// ============================================================
// UPDATE
// ============================================================

void MotionController::update()
{
    // ========================================================
    // 1. Определяем скорость коленвала
    // ========================================================

    updateValSpeed();

    // ========================================================
    // 2. Определяем состояние
    // ========================================================

    updateState();

    // ========================================================
    // 3. Частота управления
    // ========================================================

    uint32_t now =
        millis();

    if (now - _lastControl <
        _controlPeriodMs)
    {
        return;
    }

    _lastControl =
        now;

    // ========================================================
    // 4. Считываем тормоза
    // ========================================================

    bool leftBrake;
    bool rightBrake;

    readBrakes(
        leftBrake,
        rightBrake);

    // ========================================================
    // 5. ДВА ТОРМОЗА
    //
    // Принудительная новая база.
    // ========================================================

    if (leftBrake && rightBrake)
    {
        createNewBase();

        _turning = false;

        return;
    }

    // ========================================================
    // 6. TURNING
    //
    // Поворот возможен только когда:
    //
    // вал остановлен
    // +
    // мы в turn zone
    // +
    // нажат один тормоз.
    // ========================================================

    bool zone =
        _phase.inTurnZone(
            _encoder.valRawAngle());

    if (_stopped &&
        zone &&
        (leftBrake || rightBrake))
    {
        updateTurning();

        return;
    }

    // ========================================================
    // 7. WALKING
    //
    // Если вал движется — только ходьба.
    // ========================================================

    if (_walking)
    {
        _turning = false;

        updateWalking();

        return;
    }

    // ========================================================
    // 8. STOPPED
    //
    // Вал стоит.
    // Поворота нет.
    //
    // Ничего не отправляем.
    // ODrive удерживает последнюю позицию.
    // ========================================================

    _turning = false;
}