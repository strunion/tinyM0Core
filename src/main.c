#include "stm32f0xx.h"
#include "tinyM0Core.h"

uint32_t proc1Stack[64];
uint32_t proc2Stack[64];
void proc1(void);
void proc2(void);

void proc1(void){
    while(1){
            for(int i = 0; i < 20; i++){
            GPIOA->BSRR |= GPIO_BSRR_BR_5;
            GPIOA->BSRR |= GPIO_BSRR_BS_5;
        }
        yield();
    }
}

void proc2(void){
    while(1){
        for(int i = 0; i < 20; i++){
            GPIOA->BSRR |= GPIO_BSRR_BR_6;
            GPIOA->BSRR |= GPIO_BSRR_BS_6;
        }
        yield();
    }
}

int main(void){
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;
    GPIOA->MODER |= GPIO_MODER_MODER6_1 | GPIO_MODER_MODER5_0;

    osCreateThread(proc1, proc1Stack, sizeof(proc1Stack));
    osCreateThread(proc2, proc2Stack, sizeof(proc1Stack));
    osStart();

    while(1){

    }
}
