#include <stdint.h>
#include <stdbool.h>
#include <pru_cfg.h>
#include "resource_table_empty.h"
#include "sharedDataStruct.h"

#define STR_LEN         14       // # LEDs in our string
#define oneCyclesOn     700/5   // Stay on 700ns
#define oneCyclesOff    800/5
#define zeroCyclesOn    350/5
#define zeroCyclesOff   600/5
#define resetCycles     60000/5 // Must be at least 50u, use 60u

// #define STR_LEN         14       // # LEDs in our string
// #define oneCyclesOn     900/5   // Stay on 700ns
// #define oneCyclesOff    350/5
// #define zeroCyclesOn    350/5
// #define zeroCyclesOff   900/5
// #define resetCycles     60000/5 // Must be at least 50u, use 60u


// P8_11 for output (on R30), PRU0
#define DATA_PIN 15       // Bit number to output on

// GPIO Configuration
// ----------------------------------------------------------
volatile register uint32_t __R30;   // output GPIO register
volatile register uint32_t __R31;   // input GPIO register

// GPIO Output: P8_12 = pru0_pru_r30_14 
//   = LEDDP2 (Turn on/off right 14-seg digit) on Zen cape
#define DIGIT_ON_OFF_MASK (1 << 14)
// GPIO Input: P8_15 = pru0_pru_r31_15 
//   = JSRT (Joystick Right) on Zen Cape



// Shared Memory Configuration
// -----------------------------------------------------------
#define THIS_PRU_DRAM       0x00000         // Address of DRAM
#define OFFSET              0x200           // Skip 0x100 for Stack, 0x100 for Heap (from makefile)
#define THIS_PRU_DRAM_USABLE (THIS_PRU_DRAM + OFFSET)

// This works for both PRU0 and PRU1 as both map their own memory to 0x0000000
volatile sharedMemStruct_t *pSharedMemStruct = (volatile void *)THIS_PRU_DRAM_USABLE;

void initialize_LED()
{
    for (int i = 0; i < STR_LEN; i++) {
 
        // RGB
        pSharedMemStruct->colour[i] = 0x00000f; // Blue
    }
}

void write_LED()
{
    __delay_cycles(resetCycles);

        for(int j = 0; j < STR_LEN; j++) {
            for(int i = 23; i >= 0; i--) {
                if(pSharedMemStruct->colour[j] & ((uint32_t)0x1 << i)) {
                    __R30 |= 0x1<<DATA_PIN;      // Set the GPIO pin to 1
                    __delay_cycles(oneCyclesOn-1);
                    __R30 &= ~(0x1<<DATA_PIN);   // Clear the GPIO pin
                    __delay_cycles(oneCyclesOff-2);
                } else {
                    __R30 |= 0x1<<DATA_PIN;      // Set the GPIO pin to 1
                    __delay_cycles(zeroCyclesOn-1);
                    __R30 &= ~(0x1<<DATA_PIN);   // Clear the GPIO pin
                    __delay_cycles(zeroCyclesOff-2);
                }
            }
        }

        // Send Reset
        __R30 &= ~(0x1<<DATA_PIN);   // Clear the GPIO pin
        __delay_cycles(resetCycles);
}


void main(void)
{   
    // Clear SYSCFG[STANDBY_INIT] to enable OCP master port
    CT_CFG.SYSCFG_bit.STANDBY_INIT = 0;

    // Initialize:
    pSharedMemStruct->isLedOn = true;


    initialize_LED();

    while(true) {
        write_LED();

        // __halt();
    }
}

