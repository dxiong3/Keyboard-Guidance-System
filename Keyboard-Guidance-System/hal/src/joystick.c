// Joystick Functions

#include "hal/joystick.h"
#include "hal/runCommand.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>

#include "timeDelay.h"
#include "midiController.h"
#include "shutdown.h"

static pthread_t joystickThread;
static int currentMode = 1;

// Source: Dr Brian Fraser
struct DirectionInfo {
    const char * name;
    const int pinNumber;
    const char * pathToDirection;
    const char * gpioCommand;
    const char * pathToReadValue;
};

// paths for all GPIO pins
#define BASEPATH "/sys/class/gpio/"
const struct DirectionInfo directions[] = {
    {"Up", 26, BASEPATH "gpio26/direction", "config-pin p8.14 gpio", BASEPATH "gpio26/value"},
    {"Right", 47, BASEPATH "gpio47/direction", "config-pin p8.15 gpio", BASEPATH "gpio47/value"},
    {"Down", 46, BASEPATH "gpio46/direction", "config-pin p8.16 gpio", BASEPATH "gpio46/value"},
    {"Left", 65, BASEPATH "gpio65/direction", "config-pin p8.18 gpio", BASEPATH "gpio65/value"},
    {"Center", 27, BASEPATH "gpio27/direction", "config-pin p8.17 gpio", BASEPATH "gpio27/value"}
};

static enum joystickDirections Joystick_directionPressed();

static void debounceSleepTime(int prevCommand, int currCommand);

static void * joystickThreadFunction()
{   
    int currentJoystickPress;

    while(!Shutdown_isShutdown()) {
        // Functions called in this loop should only be low level functions, this is mostly for testing purposes
        // There is Joystick_isDirectionPressed() and Joystick_isAnyPressed() for higher level module function calls to the joystick
        currentJoystickPress = Joystick_directionPressed();
        switch (currentJoystickPress) {
            case JOYSTICK_CENTER:
                printf("Center Pressed\n");
                break;
            case JOYSTICK_UP:
                printf("Restarting Song\n");
                // restart song
                MidiController_setSong(MidiController_getCurrentSong());
                sleepForMs(300);
                break;
            case JOYSTICK_DOWN:
                printf("Exiting Program\n");
                Shutdown_triggerShutdown();
                break;
            case JOYSTICK_LEFT:
                // cycle the songs left
                printf("Switching song to...\n");
                MidiController_cycleSongLeft();
                sleepForMs(300);
                break;
            case JOYSTICK_RIGHT:
                // cycle the songs right
                printf("Switching song to...\n");
                MidiController_cycleSongRight();
                sleepForMs(300);
                break;
            default:
                break;
        }
    }
    return NULL;
}

// sets the GPIO pins for joystick controls
static void writePinFile(const char * GPIOFILE, char * mode)
{
    // Makes sure that the argument mode is only called with in or out

    assert(strcmp(mode, "in") == 0 || strcmp(mode, "out") == 0);
    FILE * writePinFile = fopen(GPIOFILE, "w");

    if (writePinFile == NULL) {

        printf("ERROR OPENING %s.\n", GPIOFILE);
        exit(1);
    }

    int charWritten = fprintf(writePinFile, mode);
    if (charWritten <= 0) {
        printf("ERROR WRITING DATA\n");
        exit(1);
    }

    fclose(writePinFile);
}

// init and cleanup
void Joystick_init(void)
{
    for (int x = 0; x < JOYSTICK_MAX_NUMBER_DIRECTIONS; x++) {
        // Sets pins to GPIO mode
        runCommand(directions[x].gpioCommand);
        // Writes pins to input mode
        writePinFile(directions[x].pathToDirection, "in");
    }

    int retStatus = pthread_create(&joystickThread, NULL, &joystickThreadFunction, NULL);
    if (retStatus != 0){
        fprintf(stderr, "Error in joystick thread\n");
    }
}

void Joystick_cleanup(void) {
    int retStatus = pthread_join(joystickThread, NULL);
    printf("Shutting down joystick thread.\n");
    if (retStatus != 0){
        fprintf(stderr, "Error in joining thread\n");
    }
}

// function to check if something has been pressed in a given path
static bool isButtonPressed(const char *fileName)
{   
    FILE *pFile = fopen(fileName, "r");
    if (pFile == NULL) {
        printf("ERROR: Unable to open file (%s) for read\n", fileName);
        exit(-1);
    }
    // Read string (line)
    const int MAX_LENGTH = 1024;
    char buff[MAX_LENGTH];
    fgets(buff, MAX_LENGTH, pFile);
    // Close
    fclose(pFile);
    // printf("Read: '%s'\n", buff);
    // Return true if button is pressed, else false
    if (buff[0] == '0') {
        return true;
    }
    return false;
}


bool Joystick_isDirectionPressed(enum joystickDirections direction)
{   
    return isButtonPressed(directions[direction].pathToReadValue);
}

// check if any joystick is pressed
bool Joystick_isAnyPressed(void)
{
    bool anyButtonPressed = false;
    for (int joystickDirection = 0; joystickDirection < JOYSTICK_MAX_NUMBER_DIRECTIONS; joystickDirection++) {
        bool isDirectionPressed = Joystick_isDirectionPressed(joystickDirection);
        if (isDirectionPressed == true) {
            anyButtonPressed = true;
        }
    }
    return anyButtonPressed;
}

// get the direction the joystick is pressed in
static enum joystickDirections Joystick_directionPressed() {
    for (int joystickDirection = 0; joystickDirection < JOYSTICK_MAX_NUMBER_DIRECTIONS; joystickDirection++) {
        if(Joystick_isDirectionPressed(joystickDirection)) {
            return joystickDirection;
        }
    }
    // Else return none if no direction is pressed
    return JOYSTICK_NONE;
}