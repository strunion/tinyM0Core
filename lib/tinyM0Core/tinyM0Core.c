#include "tinyM0Core.h"
#include "stm32f0xx.h"
#include "stddef.h"

extern volatile uint32_t tick;

#define NUM_OF_SYSTEM_REGS                                  12
#define REG_SIZE                                            4
#define MINIMUM_STACK_SIZE                                  (NUM_OF_SYSTEM_REGS * REG_SIZE)
#define MAGIC_SAUCE                                         0x55555555

tinyThread_t tinyThread[MAX_THREADS];
uint32_t curThread = 0;

__STATIC_FORCEINLINE
void pushCtx(){//18
    asm (
        "mov  r0, r8        \n\t"
        "mov  r1, r9        \n\t"
        "mov  r2, r10       \n\t"
        "mov  r3, r11       \n\t"
        "push {r0-r7,lr}    \n\t"
    );
}

__STATIC_FORCEINLINE
void popCtx(){//18
    asm (
        "pop  {r0-r7, lr}   \n\t"
        "mov  r8, r0        \n\t"
        "mov  r9, r1        \n\t"
        "mov  r10, r2       \n\t"
        "mov  r11, r3       \n\t"
        "bx	  lr            \n\t"
    );
}

__attribute__((naked)) __attribute__((noinline))
void yield(){
    pushCtx();
    uint32_t *ptr = &tinyThread[curThread].stackPointer;
    asm (
        "mrs r0, psp        \n\t"
        "ldr r1, =ptr       \n\t"
        "str r0, [r1]       \n\t"
        ::: "r0", "r1"
    );



    asm (
        "mov r0, #0         \n\t"
        "msr control, r0    \n\t"
        "mrs r0, psp        \n\t"
        ::: "r0"
    );
    popCtx();
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

tinyThread_t *osNextThread(void) {
    curThread++;
    while(1){
        tinyThread_t *thr = &tinyThread[curThread];
        switch (thr->state) {
            case OS_EMPTY: break;
            case OS_SLEEP: break;
            case OS_RUN:
                break;
            case OS_DELAY:
                if((int32_t)(tick - thr->tim) > 0)
                    return thr;
                break;
            case OS_WAIT_MATCH:
                if((*thr->uPtr & thr->mask) == thr->match)
                    return thr;
                break;
            case OS_WAIT_RANGE:
                if(*thr->uPtr - thr->subtrahend < thr->maxVal)
                    return thr;
                break;
            default: break;
        }
        if (++curThread == MAX_THREADS) curThread = 0;
    }
}

void osStart(void){
    while(1){
        for(int i = 0; i < MAX_THREADS; i++){
            switch(tinyThread[i].state){
                case OS_EMPTY: break;
                case OS_SLEEP: break;
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
                    if(*tinyThread[i].uPtr - tinyThread[i].subtrahend < tinyThread[i].maxVal)
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
}

void osTim(uint32_t d){
    tinyThread[curThread].state = OS_DELAY;
    tinyThread[curThread].tim = d;
    yield();
}

void osWaitMatch(uint32_t* p, uint32_t mask, uint32_t match){
    tinyThread[curThread].state = OS_WAIT_MATCH;
    tinyThread[curThread].uPtr = p;
    tinyThread[curThread].mask = mask;
    tinyThread[curThread].match = match & mask;
    yield();
}

void osWaitRange(uint32_t* p, uint32_t min, uint32_t max){
    tinyThread[curThread].state = OS_WAIT_RANGE;
    tinyThread[curThread].uPtr = p;
    tinyThread[curThread].subtrahend = min;
    tinyThread[curThread].maxVal = max - min;
    yield();
}

void osRun(uint8_t t){
    tinyThread[t].state = OS_RUN;
}

void osStop(uint8_t t){
    tinyThread[t].state = OS_SLEEP;
}

void osSelfStop(){
    tinyThread[curThread].state = OS_SLEEP;
    yield();
}
