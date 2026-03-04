#pragma once

enum Zone {
    ZONE_DEAD = 0,
    ZONE_LEFT,
    ZONE_RIGHT
};

void joystickInit();

void joystickButtonInit();          // новая функция
bool isJoystickButtonPressed();     // новая функция

Zone readJoystick();