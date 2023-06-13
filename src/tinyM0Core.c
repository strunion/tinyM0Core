#include "tinyM0Core.h"

tinyThread_t tinyThread[MAX_THREADS];

void __attribute__((naked)) osStart(void){
    static uint8_t currentThread;
    while(1){
        switch(tinyThread[currentThread].state){
            case EMPTY:
                break;  // TODO
            case NEW:
                tinyThread[currentThread].state = RUN;
                tinyThread[currentThread].proc();
                break;
            case RUN:
                break;
        }
    }
}

int osCreateThread(tinyProc_t proc, uint32_t* stack, uint32_t stackSize){
    if(stack == 0 || stackSize <= 16*4) return -1;
    if(stackSize & 0x3) return -2;
    stackSize /= 4;

    int threadId = 0;
    for(; threadId < MAX_THREADS; threadId++)
        if(tinyThread[threadId].state == EMPTY)
            break;
    if(threadId == MAX_THREADS) return -3;

    for(int i = 0; i < stackSize - 16; i++) stack[i] = 0x55555555;

    uint32_t* stackTmp = stack+stackSize-16;
    stackTmp[8] = 0xFFFFFFFD;
    stackTmp[15] = (uint32_t)proc;
    stackTmp[16] = 0x01000000;

    tinyThread[threadId].proc = proc;
    tinyThread[threadId].stackPointer = stackTmp;
    tinyThread[threadId].state = NEW;
    return threadId;
}

void yield(void){

}
