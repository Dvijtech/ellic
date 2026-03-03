#pragma once

enum Zone {
    ZONE_DEAD = 0,
    ZONE_LEFT,
    ZONE_RIGHT
};

void joystickInit();
Zone readJoystick();