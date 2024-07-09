#pragma once
#include <stdint.h>
#include "stm32f0xx.h"

#define MAX_THREADS 5

typedef void (*tinyProc_t)(void);


#define WITH(mutex) for(int _i_ = (mutexLock(mutex), 0); _i_ == 0; _i_ = (mutexUnlock(mutex), -1))
typedef volatile uint8_t mutex_t;

typedef enum{
    OS_EMPTY,
    OS_READY,
    OS_RUN,
    OS_SLEEP,
    OS_DELAY,
    OS_WAIT_MATCH
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
        uint32_t mask;
    };
    union {
        uint32_t arg3;
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
void osThreadRun(uint8_t t);
void osThreadStop(uint8_t t);
void mutexLock(mutex_t* m);
void mutexUnlock(mutex_t* m);
