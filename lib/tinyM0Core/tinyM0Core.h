#pragma once
#include <stdint.h>
#include "stm32f0xx.h"

#define MAX_THREADS 5

typedef void (*tinyProc_t)(void);

typedef enum{
    OS_EMPTY,
    OS_SLEEP,
    OS_RUN,
    OS_DELAY,
    OS_WAIT_MATCH,
    OS_WAIT_RANGE
} tinyThreadState_t;

enum {
    OS_NULL_STACK_PTR_ERR = -1,
    OS_STACK_SIZE_ERR = -2,
    OS_STACK_ALIGN_ERR = -3,
    OS_TOO_MANY_THREADS_ERR = -4
};

typedef struct{
    union {
        uint32_t arg1;
        uint32_t tim;
        void* ptr;
        uint32_t* uPtr;
    };
    union {
        uint32_t arg2;
        uint32_t subtrahend;
        uint32_t mask;
    };
    union {
        uint32_t arg3;
        uint32_t maxVal;
        uint32_t match;
    };
    void* stackPointer;
    tinyThreadState_t state;
} tinyThread_t;

void osStart(void);
int osCreateThread(tinyProc_t proc, uint32_t* stack, uint32_t stackSize);
void yield();
void osDelay(uint32_t d);
void osTim(uint32_t d);
void osWaitMatch(uint32_t* p, uint32_t mask, uint32_t match);
void osWaitRange(uint32_t* p, uint32_t min, uint32_t max);
void osRun(uint8_t t);
void osStop(uint8_t t);
void osSelfStop();
