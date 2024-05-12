#include "system.h"
#include "config.h"
#include "tinyM0Core.h"

void rccInit(void){
    FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY;

    RCC->CR = RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    RCC->CFGR = RCC_CFGR_PLLSRC_HSE_PREDIV | RCC_CFGR_PLLMUL6;

    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while((RCC->CFGR & RCC_CFGR_SWS)!= RCC_CFGR_SWS_PLL);

    RCC->AHBENR |= RCC_AHBENR_GPIOFEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOAEN | RCC_AHBENR_DMAEN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN | RCC_APB2ENR_SYSCFGEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM14EN;
}

void gpioInit(void){
    // PA10 - DEBUG_RX      - USART1_RX
    // PA9  - DEBUG_TX      - USART1_TX

    // 0 - GPIO_OUT
    // 1 - Alternate function
    // Msk - Analog mode

    GPIOA->MODER |=GPIO_MODER_MODER10_1
                 | GPIO_MODER_MODER9_1
                 | GPIO_MODER_MODER4_0;
    GPIOA->AFR[1] |= 0x00000110;
}

void debugWrite(uint8_t d){
    while(!(USART1->ISR & USART_ISR_TXE))
        osWaitMatch((void*)(&(USART1->ISR)), USART_ISR_TXE, USART_ISR_TXE);
    USART1->TDR = d;
}

void debugInit(void){
    USART1->BRR = F_CPU / DEBUG_BAUD;
    USART1->CR1 = USART_CR1_TE | USART_CR1_RE;
    USART1->CR1 |= USART_CR1_UE;
    xdev_out(debugWrite);
}

void tim14Init(){
    TIM14->PSC = 47;
    TIM14->EGR |= TIM_EGR_UG;
    TIM14->CR1 |= TIM_CR1_CEN;
}

void nvicInit(void){
    NVIC_SetPriority(SysTick_IRQn, 3);
    SysTick_Config(F_CPU / 1000 - 1);
}

void sysInit(void){
    rccInit();
    gpioInit();
    debugInit();
    tim14Init();
    nvicInit();
}
