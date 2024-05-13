#include "tinyM0Core.h"
#include "stm32f0xx.h"
#include "stddef.h"

extern volatile uint32_t tick;

#define NUM_OF_SYSTEM_REGS                                  9
#define REG_SIZE                                            4
#define MINIMUM_STACK_SIZE                                  (NUM_OF_SYSTEM_REGS * REG_SIZE)
#define MAGIC_SAUCE                                         0x55555555

tinyThread_t tinyThread[MAX_THREADS];
uint32_t curThread = 0;

__STATIC_FORCEINLINE
void pushCtx(){//18
    asm (
        "push {r4-r7,lr}    \n\t"
        "mov  r4, r8        \n\t"
        "mov  r5, r9        \n\t"
        "mov  r6, r10       \n\t"
        "mov  r7, r11       \n\t"
        "push {r4-r7}       \n\t"
    );
}

__STATIC_FORCEINLINE
void popCtx(){//18
    asm (
        "pop  {r4-r7}       \n\t"
        "mov  r8, r4        \n\t"
        "mov  r9, r5        \n\t"
        "mov  r10, r6       \n\t"
        "mov  r11, r7       \n\t"
        "pop  {r4-r7, pc}   \n\t"
    );
}

__attribute__((naked))
void* toThread(void* stack){
    pushCtx();
    asm (
        "msr psp, r0        \n\t"
        "mov r0, #3         \n\t"
        "msr control, r0    \n\t"
        :::"r0"
    );
    popCtx();
}

__attribute__((naked)) __attribute__((noinline))
void yield(){
    pushCtx();
    asm (
        "mov r0, #0         \n\t"
        "msr control, r0    \n\t"
        "mrs r0, psp        \n\t"
        :::"r0"
    );
    popCtx();
}

void goToThread(int threadId){
    curThread = threadId;
    tinyThread[threadId].stackPointer = toThread(tinyThread[threadId].stackPointer);
}

void osStart(void){
    while(1){
        for(int i = 0; i < MAX_THREADS; i++){
            switch(tinyThread[i].state){
                case OS_RUN:
                    goToThread(i);
                    break;
                case OS_DELAY:
                    if((int32_t)(tick - tinyThread[i].tim) > 0)
                        goToThread(i);
                    break;
                case OS_WAIT_MATCH:
                    if((*tinyThread[i].uPtr & tinyThread[i].mask) == tinyThread[i].match)
                        goToThread(i);
                    break;
                case OS_WAIT_RANGE:
                    if((*tinyThread[i].uPtr - tinyThread[i].min) < tinyThread[i].shiftedMax)
                        goToThread(i);
                    break;
                default: break;
            }
        }
    }
}

int osCreateThread(tinyProc_t proc, uint32_t* stack, uint32_t stackSize){
    if(stack == NULL) return OS_NULL_STACK_PTR_ERR;
    if(stackSize < MINIMUM_STACK_SIZE) return OS_STACK_SIZE_ERR;
    if(stackSize & 0x3) return OS_STACK_ALIGN_ERR;

    int reg_num = stackSize / REG_SIZE;

    int threadId = 0;
    for(; threadId < MAX_THREADS; threadId++)
        if(tinyThread[threadId].state == OS_EMPTY)
            break;
    if(threadId == MAX_THREADS) return OS_TOO_MANY_THREADS_ERR;

    for(int i = 0; i < reg_num - NUM_OF_SYSTEM_REGS; i++) stack[i] = MAGIC_SAUCE;

    stack[reg_num - 1] = (uint32_t)proc;

    tinyThread[threadId].stackPointer = stack + reg_num - NUM_OF_SYSTEM_REGS;
    tinyThread[threadId].state = OS_RUN;
    return threadId;
}

void osDelay(uint32_t d){
    tinyThread[curThread].state = OS_DELAY;
    tinyThread[curThread].tim = tick + d;
    yield();
    tinyThread[curThread].state = OS_RUN;
}

void osTim(uint32_t d){
    tinyThread[curThread].state = OS_DELAY;
    tinyThread[curThread].tim = d;
    yield();
    tinyThread[curThread].state = OS_RUN;
}

void osWaitMatch(uint32_t* p, uint32_t mask, uint32_t match){
    tinyThread[curThread].state = OS_WAIT_MATCH;
    tinyThread[curThread].uPtr = p;
    tinyThread[curThread].mask = mask;
    tinyThread[curThread].match = match & mask;
    yield();
    tinyThread[curThread].state = OS_RUN;
}

void osWaitRange(uint32_t* p, uint32_t min, uint32_t max){
    tinyThread[curThread].state = OS_WAIT_RANGE;
    tinyThread[curThread].uPtr = p;
    tinyThread[curThread].min = min;
    tinyThread[curThread].shiftedMax = max - min;
    yield();
    tinyThread[curThread].state = OS_RUN;
}

void osRun(uint8_t t){
    tinyThread[t].state = OS_RUN;
}

void osStop(uint8_t t){
    tinyThread[t].state = OS_SLEEP;
}

void mutexLock(mutex* m){
    if(*m) osWaitMatch((uint32_t*)m, 1, 0);
    *m = 1;
}

void mutexUnlock(mutex* m){
    *m = 0;
}
