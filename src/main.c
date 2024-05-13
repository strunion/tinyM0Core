#include "system.h"
#include "sysControl.h"
#include "tinyM0Core.h"

uint32_t proc1Stack[32];

__attribute__((noinline)) void my_yield(void){
    yield();
}

__attribute__((noinline)) void test_r0_failure (const char *fmt, uint32_t x)
{
    xprintf(fmt, x);
    my_yield();
    xprintf(fmt, x);
}

const char* fmt1 = "p1 = %d\n";
volatile uint32_t value1 = 123;
void proc1(void){
    while(1){
        // GPIOA->BSRR = GPIO_BSRR_BS_4;
        // osDelay(5);
        // GPIOA->BSRR = GPIO_BSRR_BR_4;
        // osDelay(5);
        test_r0_failure(fmt1, value1);
        osSelfStop();
    }
}


const char* fmt2 = "p2 = %d\n";
volatile uint32_t value2 = 312;
uint32_t proc2Stack[64];
void proc2(void){
    while(1){
        // xprintf("%d\n", tick);
        test_r0_failure(fmt2, value2);
        osSelfStop();
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
    // osCreateThread(proc3, proc3Stack, sizeof(proc3Stack));
    osStart();

    while(1);
}

