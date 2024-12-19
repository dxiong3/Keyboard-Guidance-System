// Main driver of keyboard guidance system

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>

#include "shutdown.h"
#include "udp.h"

#include <hal/audioGenerator.h>
#include <hal/ledDriver.h>
#include <hal/colours.h>
#include <hal/midiReader.h>
#include <hal/joystick.h>
#include <hal/segDisplay.h>
#include <midiController.h>

int main(void){
    LED_init();
    Joystick_init();
    audioGenerator_init();
    MidiReader_init();
    MidiController_init();
    SegmentDisplay_init();
    UDP_init();
    Shutdown_init();

	Shutdown_waitForShutdown();
    Shutdown_cleanup();

    UDP_cleanup();
    LED_cleanup();
    Joystick_cleanup();
    audioGenerator_cleanup();
    MidiReader_cleanup();
    MidiController_cleanup();
    SegmentDisplay_cleanup();

    return 0;
    
}
