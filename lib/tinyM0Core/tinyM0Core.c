#include "tinyM0Core.h"
#include "stm32f0xx.h"

extern volatile uint32_t tick;

tinyThread_t tinyThread[MAX_THREADS];
uint32_t curThread = 0;

__STATIC_FORCEINLINE
void pushCtx(){//18
    asm (
        "push {r1-r7,lr}    \n\t"
        "mov  r1, r8        \n\t"
        "mov  r2, r9        \n\t"
        "mov  r3, r10       \n\t"
        "mov  r4, r11       \n\t"
        "push {r1-r4}       \n\t"
    );
}

__STATIC_FORCEINLINE
void popCtx(){//18
    asm (
        "pop  {r1-r4}       \n\t"
        "mov  r8, r1        \n\t"
        "mov  r9, r2        \n\t"
        "mov  r10, r3       \n\t"
        "mov  r11, r4       \n\t"
        "pop  {r1-r7, pc}   \n\t"
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

void* toThread(void* stack);

void goToThread(int threadId){
    curThread = threadId;
    tinyThread[threadId].stackPointer = toThread(tinyThread[threadId].stackPointer);
}

void osStart(void){
    while(1){
        for(int i = 0; i < MAX_THREADS; i++){
            switch(tinyThread[i].state){
                case EMPTY: break;
                case SLEEP: break;
                case RUN:
                    goToThread(i);
                    break;
                case DELAY:
                    if((int32_t)(tick - tinyThread[i].tim) > 0)
                        goToThread(i);
                    break;
                case WAIT_MATCH:
                    if((*tinyThread[i].uPtr & tinyThread[i].mask) == tinyThread[i].match)
                        goToThread(i);
                    break;
                case WAIT_RANGE:
                    if((tinyThread[i].minVal < tinyThread[i].maxVal) ?
                       (*tinyThread[i].uPtr >= tinyThread[i].minVal &&
                       *tinyThread[i].uPtr <= tinyThread[i].maxVal) :
                       (*tinyThread[i].uPtr > tinyThread[i].minVal ||
                       *tinyThread[i].uPtr < tinyThread[i].maxVal))
                        goToThread(i);
                    break;
                default: break;
            }
        }
    }
}

int osCreateThread(tinyProc_t proc, uint32_t* stack, uint32_t stackSize){
    if(stack == 0 || stackSize <= 12*4) return -1;
    if(stackSize & 0x3) return -2;
    stackSize /= 4;

    int threadId = 0;
    for(; threadId < MAX_THREADS; threadId++)
        if(tinyThread[threadId].state == EMPTY)
            break;
    if(threadId == MAX_THREADS) return -3;

    for(int i = 0; i < stackSize - 12; i++) stack[i] = 0x55555555;

    stack[stackSize - 1] = (uint32_t)proc;

    tinyThread[threadId].stackPointer = stack + stackSize - 12;
    tinyThread[threadId].state = RUN;
    return threadId;
}

void osDelay(uint32_t d){
    tinyThread[curThread].state = DELAY;
    tinyThread[curThread].tim = tick + d;
    yield();
}

void osTim(uint32_t d){
    tinyThread[curThread].state = DELAY;
    tinyThread[curThread].tim = d;
    yield();
}

void osWaitMatch(uint32_t* p, uint32_t mask, uint32_t match){
    tinyThread[curThread].state = WAIT_MATCH;
    tinyThread[curThread].uPtr = p;
    tinyThread[curThread].mask = mask;
    tinyThread[curThread].match = match & mask;
    yield();
}

void osWaitRange(uint32_t* p, uint32_t min, uint32_t max){
    tinyThread[curThread].state = WAIT_RANGE;
    tinyThread[curThread].uPtr = p;
    tinyThread[curThread].minVal = min;
    tinyThread[curThread].maxVal = max;
    yield();
}

void osRun(uint8_t t){
    tinyThread[t].state = RUN;
}

void osStop(uint8_t t){
    tinyThread[t].state = EMPTY;
}
