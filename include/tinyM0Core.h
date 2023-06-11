#pragma once
#include <stdint.h>

typedef void (*tinyProc_t)(void);

typedef enum{
    NEW,
    RUN,
} tinyThreadState_t;

typedef struct{
    tinyProc_t proc;
    uint8_t* stack;
} tinyThreadConfig_t;

typedef struct{
    tinyThreadState_t state;
    uint32_t stackPointer;
} tinyThread_t;

void osStart(void);
void yield(void);
