#include "system.h"
#include "sysControl.h"
#include "tinyM0Core.h"

uint32_t proc1Stack[32];
void proc1(void){
    while(1){
        GPIOA->BSRR = GPIO_BSRR_BS_4;
        osDelay(5);
        GPIOA->BSRR = GPIO_BSRR_BR_4;
        osDelay(5);
    }
}

uint32_t proc2Stack[64];
void proc2(void){
    while(1){
        xprintf("%d\n", tick);
        yield();
    }
}

uint32_t proc3Stack[64];
void proc3(void){
    while(1){
        for(int i = 0; i < sizeof(proc2Stack) / sizeof(proc2Stack[0]); i++)
            xprintf("%d:%08X\n", i, proc2Stack[i]);
        xprintf("\n");
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

