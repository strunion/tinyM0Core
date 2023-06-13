#pragma once
#include <stdint.h>

#define MAX_THREADS 5

typedef void (*tinyProc_t)(void);

typedef enum{
    EMPTY,
    NEW,
    RUN
} tinyThreadState_t;

typedef struct{
    tinyProc_t proc;
    void* stackPointer;
    tinyThreadState_t state;
} tinyThread_t;

void osStart(void);
int osCreateThread(tinyProc_t proc, uint32_t* stack, uint32_t stackSize);
void yield(void);
