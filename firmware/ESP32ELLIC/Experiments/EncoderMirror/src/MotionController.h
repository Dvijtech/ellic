#pragma once

#include "PhaseDetector.h"


class MotionController
{

public:


    void begin();


    // расчет момента помощи
    void update(
        float angle,
        GaitPhase phase
    );


    float leftTorque();

    float rightTorque();



private:


    float _leftTorque = 0;

    float _rightTorque = 0;


    float _targetLeft = 0;

    float _targetRight = 0;



    // максимальный момент помощи

    float _maxTorque = 0.15f;



    // скорость нарастания момента

    float _slew = 0.01f;



    float smoothTorque(
        float current,
        float target
    );



    float sigmoid(
        float x
    );


};