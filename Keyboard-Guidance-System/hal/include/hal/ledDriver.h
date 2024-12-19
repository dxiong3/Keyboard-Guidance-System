// Module to control the custom LEDs

#ifndef _LEDDRIVER_H_
#define _LEDDRIVER_H_

// Changes colours of LEDs
void changeColourLED(unsigned int colour, int LEDNum);
void turnOnAllLEDs(unsigned int colour);
void turnOffAllLEDs(void);
// Animation when user finishes playing a song
void LED_finishAnimation(void);
void LED_triggerBlink(unsigned int colour, int LEDNum);

// Cleanup and init functions for LEDs
void LED_init(void);
void LED_cleanup(void);

#endif