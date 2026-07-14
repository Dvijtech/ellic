#include "MotionController.h"

#include <math.h>



void MotionController::begin()
{

    _leftTorque = 0;

    _rightTorque = 0;

}



//----------------------------------
// сигмоида
//----------------------------------

float MotionController::sigmoid(float x)
{

    return 
        1.0f /
        (1.0f + exp(-x));

}



//----------------------------------
// плавное изменение момента
//----------------------------------

float MotionController::smoothTorque(
        float current,
        float target
)
{

    float diff =
        target - current;


    if(diff > _slew)
        diff = _slew;


    if(diff < -_slew)
        diff = -_slew;



    return current + diff;

}



//----------------------------------

void MotionController::update(
        float angle,
        GaitPhase phase
)
{


    //--------------------------------
    // базовое состояние
    //--------------------------------

    _targetLeft = 0;

    _targetRight = 0;



    //--------------------------------
    // помощь шагу
    //--------------------------------


    switch(phase)
    {


        //----------------------------

        case GaitPhase::LEFT_START:
        {

            float k =
                sigmoid(
                    (15-angle)*0.25f
                );


            _targetLeft =
                _maxTorque*k;


            break;
        }



        //----------------------------

        case GaitPhase::LEFT_PUSH:
        {


            float k =
                sigmoid(
                    (angle-70)*0.15f
                );


            _targetLeft =
                _maxTorque*k;


            break;

        }



        //----------------------------

        case GaitPhase::RIGHT_START:
        {


            float k =
                sigmoid(
                    (15-angle)*0.25f
                );


            _targetRight =
                _maxTorque*k;


            break;

        }



        //----------------------------

        case GaitPhase::RIGHT_PUSH:
        {


            float k =
                sigmoid(
                    (angle-70)*0.15f
                );


            _targetRight =
                _maxTorque*k;


            break;

        }



        default:

            break;

    }




    //--------------------------------
    // плавность
    //--------------------------------


    _leftTorque =
        smoothTorque(
            _leftTorque,
            _targetLeft
        );



    _rightTorque =
        smoothTorque(
            _rightTorque,
            _targetRight
        );



}




float MotionController::leftTorque()
{
    return _leftTorque;
}



float MotionController::rightTorque()
{
    return _rightTorque;
}