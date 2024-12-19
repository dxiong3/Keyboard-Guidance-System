// File to keep track of timers and delay functions

#ifndef _TIMEDELAY_H_
#define _TIMEDELAY_H_

// Given function for waiting number of milliseconds
void sleepForMs(long long delayInMs);

// Sleeps for seconds, nanoseconds
void customSleep(long seconds, long nanoseconds);

// Get current time in milliseconds
long long getTimeInMs(void);

#endif