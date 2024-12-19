// Module to control the shutdown of program
// Uses mutex to lock shutdown if not triggered

#ifndef _SHUTDOWN_H_
#define _SHUTDOWN_H_

#include <stdbool.h>

// Initialize/cleanup the shutdown module
void Shutdown_init(void);
void Shutdown_cleanup(void);

// Signals shutdown and can be called from any module
void Shutdown_triggerShutdown(void);

// Returns true if shutdown signal has been received
bool Shutdown_isShutdown(void);

// Waits for shutdown signal to be received
void Shutdown_waitForShutdown(void);

#endif