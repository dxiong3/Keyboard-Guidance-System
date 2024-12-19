// Module to control the custom LEDs

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <assert.h>
#include <hal/colours.h>
#include "timeDelay.h"
#include <pthread.h>

#include "shutdown.h"
#include "hal/sharedDataStruct.h"
#include "hal/pruMem.h"
#include "hal/ledDriver.h"

#define NUM_LEDS 14

#define PRU0_DRAM     0x00000      // Offset to DRAM
#define PRU_MEM_RESERVED 0x200     // Amount used by stack and heap
#define PRU0_MEM_FROM_BASE(base) ( (base) + PRU0_DRAM + PRU_MEM_RESERVED)

volatile void *pPruBase;
volatile sharedMemStruct_t *pSharedPru0;

static pthread_t ledThread;

// Flags to set for blinking
bool blink = false;
int colour;
int LEDNum;

// blinks red LED if user plays wrong noet
static void LED_blink(unsigned int colour, int LEDNum)
{
    for (int x = 0; x < 3; x++) {
        changeColourLED(colour, LEDNum);
        sleepForMs(500);
        changeColourLED(CLEAR, LEDNum);
        sleepForMs(200);
    }
}
// change the led colors according to PRU shared struct
void changeColourLED(unsigned int colour, int LEDNum)
{
    assert(LEDNum >= 0 || LEDNum <= 13);
    // Get access to shared memory for my uses
    pSharedPru0->colour[LEDNum] = colour;
}

void turnOnAllLEDs(unsigned int colour)
{
    for (int i = 0; i < NUM_LEDS; i++) {
        pSharedPru0->colour[i] = colour;
    }
}

void turnOffAllLEDs(void)
{
    for (int i = 0; i < NUM_LEDS; i++) {
        pSharedPru0->colour[i] = CLEAR;
    }
}

static void * ledThreadFunction()
{
    while(!Shutdown_isShutdown()) {
        if (blink) {
            // printf("Will trigger note now hex: %x at %d pos \n", colour, LEDNum);
            LED_blink(colour, LEDNum);
            blink = false;
        }
    }

}

// init and cleanup functions
void LED_init()
{
    pPruBase = getPruMmapAddr();
    pSharedPru0 = PRU0_MEM_FROM_BASE(pPruBase);

    int retStatus = pthread_create(&ledThread, NULL, &ledThreadFunction, NULL);
    if (retStatus != 0){
        fprintf(stderr, "Error in ledDriver thread\n");
    }
}

void LED_cleanup()
{
    // Cleanup
    turnOffAllLEDs();
    int retStatus = pthread_join(ledThread, NULL);
    if (retStatus != 0){
        fprintf(stderr, "Error in joining thread\n");
    }
    freePruMmapAddr(pPruBase);
    printf("LED cleanup done!\n");
}

void LED_finishAnimation(void)
{
    // Turn off all LEDs
    turnOffAllLEDs();
    for (int z = 0; z < 3; z++) {
        int y = 13;
        for (int x = 0; x < 14; x++) {
            changeColourLED(WHITE, x);
            changeColourLED(WHITE, y);
            sleepForMs(50);
            turnOffAllLEDs();
            y--;
        }
        turnOffAllLEDs();
    }
}

void LED_triggerBlink(unsigned int colourToTrigger, int LEDNumToTrigger)
{
    blink = true;
    LEDNum = LEDNumToTrigger;
    colour = colourToTrigger;
}