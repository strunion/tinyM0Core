#include "sysControl.h"
#include "system.h"

uint32_t volatile tick=0;
uint32_t volatile sec_d=0;
uint32_t volatile sec=0;

#define PA0_HIGH    GPIOA->BSRR = GPIO_BSRR_BS_0
#define PA0_LOW     GPIOA->BSRR = GPIO_BSRR_BR_0

void SysTick_Handler(void){
    PA0_HIGH;
    tick++;
    if(++sec_d == 10){
        sec_d = 0;
        sec++;
    }
    PA0_LOW;
    IWDG->KR = 0xAAAA;
}

void NMI_Handler(void) {
    while(1);
}

void HardFault_Handler(void){
    while(1);
}
