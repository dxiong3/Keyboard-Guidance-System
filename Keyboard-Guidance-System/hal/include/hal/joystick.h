// Module to control joystick Functions

#ifndef _JOYSTICK_H_
#define _JOYSTICK_H_

#include <stdbool.h>

// Source: Dr Brian Fraser
enum joystickDirections
{
    JOYSTICK_UP = 0,
    JOYSTICK_RIGHT,
    JOYSTICK_DOWN,
    JOYSTICK_LEFT,
    JOYSTICK_CENTER,
    JOYSTICK_MAX_NUMBER_DIRECTIONS,
    JOYSTICK_NONE
};

// init and cleanup function
void Joystick_init(void);
void Joystick_cleanup(void);

// Checks if a joystick direction is pressed
bool Joystick_isDirectionPressed(enum joystickDirections direction);
// Checks if any joystick directions are pressed
bool Joystick_isAnyPressed(void);
// Gives direction name
const char * Joystick_getDirection(enum joystickDirections direction);

#endif