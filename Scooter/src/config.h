#pragma once

// ===== Pins =====
#define PIN_JOYSTICK_X 34

#define PIN_PWM_LEFT   25
#define PIN_PWM_RIGHT  26

// пока убрал пины направления 
// #define PIN_DIR_LEFT   27 
// #define PIN_DIR_RIGHT  14

// ===== PWM config =====
#define PWM_FREQ       20000
#define PWM_RESOLUTION 8
#define PWM_MAX        255

// ===== Control params =====
#define LOOP_PERIOD_MS 20

extern int MaxPower;   // 0–100
extern int RampStep;   // 0–100
extern int Deadzone;   // percent (e.g. 30)
extern int Mode;       // 0 or 1