#include "system.h"
#include "sysControl.h"
#include "tinyM0Core.h"

mutex_t uartM;

uint32_t proc1Stack[32];
__attribute__((noreturn))
void proc1(void){
    while(1){
        GPIOA->BSRR = GPIO_BSRR_BS_4;
        osDelay(5);
        GPIOA->BSRR = GPIO_BSRR_BR_4;
        osDelay(5);
    }
}

uint32_t proc2Stack[128];
__attribute__((noreturn))
void proc2(void){
    while(1){
        WITH(&uartM){
            xprintf("%016X %020d %020lld %020llu %020llu\n",
                tick, tick, (int64_t)tick*tick, (uint64_t)tick*tick, (uint64_t)tick*tick*tick);
        }
        yield();
    }
}

uint32_t proc3Stack[64];
__attribute__((noreturn))
void proc3(void){
    while(1){
        WITH(&uartM){
            uint32_t regNum = sizeof(proc2Stack) / sizeof(proc2Stack[0]);
            for(int i = 0; i < regNum; i++)
                xprintf("%d:%08X\n", regNum - i, proc2Stack[i]);
            xprintf("\n");
        }
        osDelay(10000);
    }
}

void uartWrite(uint8_t d){
    while(!(USART1->ISR & USART_ISR_TXE)) yield();
    USART1->TDR=d;
}

int main(void){
    sysInit();

    osCreateThread(proc1, proc1Stack, sizeof(proc1Stack));
    osCreateThread(proc2, proc2Stack, sizeof(proc2Stack));
    osCreateThread(proc3, proc3Stack, sizeof(proc3Stack));
    osStart();

    while(1);
}

