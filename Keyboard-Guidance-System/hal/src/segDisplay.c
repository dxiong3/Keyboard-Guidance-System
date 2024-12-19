// Module to control the 14-seg LED

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <hal/runCommand.h>
#include "timeDelay.h"
#include <hal/writeFile.h>
#include <shutdown.h>
#include <midiController.h>

#define I2CDRV_LINUX_BUS0 "/dev/i2c-0"
#define I2CDRV_LINUX_BUS1 "/dev/i2c-1"
#define I2CDRV_LINUX_BUS2 "/dev/i2c-2"

static pthread_t segmentDisplayThread;

// Holds the 2 digits
typedef struct twoNumbers {
    int leftDigit;
    int rightDigit;
} twoNumber;

// Struct to split double digit numbers into individual digits
static struct twoNumbers splitNum(int x) {
    char str[100];
    sprintf(str, "%d", x);
    twoNumber num;
    if (x < 10 && x >= 0) {
        num.leftDigit = 0;
        num.rightDigit = x;
    } else {
        num.leftDigit = str[0] - '0';
        num.rightDigit = str[1] - '0';
    }

    return num;

}

struct numberLocationCommandNumber {
    const unsigned char commandLowerHalfB;
    const unsigned char commandUpperHalfA;
};
// Numbers are index to the count i.e. 0 = numCommands[0], 4 = numCommands[4]
const struct numberLocationCommandNumber numCommands[] = {
    {0xD0, 0xE1},
    {0x02, 0x08},
    {0x98, 0x83},
    {0xD8, 0x03},
    {0xC8, 0x22},
    {0x58, 0x23},
    {0x58, 0xA3},
    {0xC0, 0x01},
    {0xD8, 0xA3},
    {0xD8, 0x23}
};

static int initI2CBus(char * bus, int address)
{
    int i2cFileDesc = open(bus, O_RDWR);
    int result = ioctl(i2cFileDesc, I2C_SLAVE, address);
    if (result < 0) {
        perror("I2C: Unable to set I2C device to slave address.");
        exit(1);
    }
    return i2cFileDesc;
}

static void writeI2CReg(int i2cFileDesc, unsigned char regAddr, unsigned char val)
{
    unsigned char buff[2];
    buff[0] = regAddr;
    buff[1] = val;
    int res = write(i2cFileDesc, buff, 2);
    if (res != 2) {
        perror("I2C: Unable to write i2c register.");
        exit(1);
    }
}


# define I2C_DEVICE_ADDRESS 0x20

#define REG_DIRA 0x02
#define REG_DIRB 0x03
#define REG_OUTB 0x00
#define REG_OUTA 0x01

#define LEFT_PATH "/sys/class/gpio/gpio61/value"
#define RIGHT_PATH "/sys/class/gpio/gpio44/value"

#define OFF "0"
#define ON "1"

// Displays the numbers on the I2C 14-seg LED
static void displayNumber(int leftDigit, int rightDigit, int i2cFileDesc) {
    // Drive left digit
    writeFile(LEFT_PATH, OFF);
    writeFile(RIGHT_PATH, OFF);
    writeI2CReg(i2cFileDesc, REG_OUTA, numCommands[leftDigit].commandUpperHalfA);
    writeI2CReg(i2cFileDesc, REG_OUTB, numCommands[leftDigit].commandLowerHalfB);
    writeFile(LEFT_PATH, ON);
    sleepForMs(5);

    // Drive right digit
    writeFile(LEFT_PATH, OFF);
    writeFile(RIGHT_PATH, OFF);
    writeI2CReg(i2cFileDesc, REG_OUTA, numCommands[rightDigit].commandUpperHalfA);
    writeI2CReg(i2cFileDesc, REG_OUTB, numCommands[rightDigit].commandLowerHalfB);
    writeFile(RIGHT_PATH, ON);
    sleepForMs(5);
}

static void * segmentDisplayThreadFunction() {

    // configure the pins
    runCommand("config-pin P9_18 i2c");
    runCommand("config-pin P9_17 i2c");
    runCommand("echo out > /sys/class/gpio/gpio61/direction");
    runCommand("echo out > /sys/class/gpio/gpio44/direction");
    int i2cFileDesc = initI2CBus(I2CDRV_LINUX_BUS1, I2C_DEVICE_ADDRESS);
    writeI2CReg(i2cFileDesc, REG_DIRA, 0x00);
    writeI2CReg(i2cFileDesc, REG_DIRB, 0x00);

    bool exitFlag = false;

    while(!Shutdown_isShutdown()) {

        // Display current song on i2c 
        int currentSong = MidiController_getCurrentSong();
        struct twoNumbers twoNum = splitNum(currentSong);

        displayNumber(twoNum.leftDigit,twoNum.rightDigit, i2cFileDesc);
    }

    // Turns off segmentDisplay when shutting down
    writeFile(LEFT_PATH, OFF);
    writeFile(RIGHT_PATH, OFF);
    
    close(i2cFileDesc);

    printf("I2c thread done\n");
    return NULL;
}

// init and cleanup functions
void SegmentDisplay_init(void)
{  
    int retStatus = pthread_create(&segmentDisplayThread, NULL, &segmentDisplayThreadFunction, NULL);
    if (retStatus != 0){
        fprintf(stderr, "Error in creating thread\n");
    }
}
void SegmentDisplay_cleanup(void)
{
    int retStatus = pthread_join(segmentDisplayThread, NULL);
    if (retStatus != 0){
        fprintf(stderr, "Error in joining thread\n");
    }
    printf("14 seg cleanup done!\n");

}
