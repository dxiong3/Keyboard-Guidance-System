#include "shutdown.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

_Atomic bool stop = false;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Initialize/cleanup shutdown module
void Shutdown_init(void){
    pthread_mutex_lock(&mutex);
}
void Shutdown_cleanup(void){
}

// Signals shutdown
void Shutdown_triggerShutdown(void){
    stop = true;
    pthread_mutex_unlock(&mutex);
}

// Returns true if shutdown signal has been received
bool Shutdown_isShutdown(void){
    return stop;
}

// Waits for shutdown signal
void Shutdown_waitForShutdown(void){
    pthread_mutex_lock(&mutex);
    pthread_mutex_unlock(&mutex);
}