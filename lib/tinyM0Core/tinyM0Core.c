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

/// Сохранение контекста текущего потока
__STATIC_FORCEINLINE void pushCtx(){//18
    asm (
        "push {r4-r7,lr}    \n\t"
        "mov  r4, r8        \n\t"
        "mov  r5, r9        \n\t"
        "mov  r6, r10       \n\t"
        "mov  r7, r11       \n\t"
        "push {r4-r7}       \n\t"
    );
}

/// Восстановление контекста потока
__STATIC_FORCEINLINE void popCtx(){
    asm (
        "pop  {r4-r7}       \n\t"
        "mov  r8, r4        \n\t"
        "mov  r9, r5        \n\t"
        "mov  r10, r6       \n\t"
        "mov  r11, r7       \n\t"
        "pop  {r4-r7, pc}   \n\t"
    );
}

/// Переключение на новый поток
__attribute__((naked)) void* toThread(void* stack){
    pushCtx();
    asm (
        "msr psp, r0        \n\t"
        "mov r0, #3         \n\t"
        "msr control, r0    \n\t"
        :::"r0"
    );
    popCtx();
}

/// Функция передачи управления планировщику
__attribute__((naked)) __attribute__((noinline)) void yield(){
    pushCtx();
    asm (
        "mov r0, #0         \n\t"
        "msr control, r0    \n\t"
        "mrs r0, psp        \n\t"
        :::"r0"
    );
    popCtx();
}

/// Переключение на указанный поток
static void goToThread(int threadId){
    curThread = threadId;
    tinyThread[threadId].stackPointer = toThread(tinyThread[threadId].stackPointer);
}

/// Основной цикл планировщика
void osStart(void){
    while(1){
        for(int i = 0; i < MAX_THREADS; i++){
            switch(tinyThread[i].state){
                case OS_RUN:
                    goToThread(i);
                    break;
                case OS_DELAY:
                    if((int32_t)(tick - tinyThread[i].tim) >= 0)
                        goToThread(i);
                    break;
                case OS_WAIT_MATCH:
                    if((*tinyThread[i].uPtr & tinyThread[i].mask) == tinyThread[i].match)
                        goToThread(i);
                    break;
                case OS_WAIT_MATCH_OR_DELAY:
                    if((*tinyThread[i].uPtr & tinyThread[i].mask) == tinyThread[i].match
                    || (int32_t)(tick - tinyThread[i].tim) >= 0)
                        goToThread(i);

                default: break;
            }
        }
    }
}

/// Создает новый поток в системе.
/// @param proc Указатель на функцию, которая будет выполняться в новом потоке.
/// @param stack Указатель на выделенную область памяти для стека потока.
/// @param stackSize Размер выделенной области памяти для стека (в байтах).
/// @return Возвращает идентификатор созданного потока (неотрицательное число) в случае успеха.
///         В случае ошибки возвращает одно из следующих отрицательных значений:
///         - OS_NULL_STACK_PTR_ERR: если указатель на стек равен NULL.
///         - OS_STACK_SIZE_ERR: если размер стека меньше минимально допустимого.
///         - OS_STACK_ALIGN_ERR: если стек не выровнен по 4 байта.
///         - OS_TOO_MANY_THREADS_ERR: если достигнуто максимальное количество потоков.
///  @note Функция инициализирует стек потока и устанавливает его начальное состояние как OS_RUN.
int osCreateThread(tinyProc_t proc, uint32_t* stack, uint32_t stackSize){
    // Проверки параметров
    if(stack == NULL) return OS_NULL_STACK_PTR_ERR;
    if(stackSize < MINIMUM_STACK_SIZE) return OS_STACK_SIZE_ERR;
    if((stackSize & 0x3) || ((uint32_t)stack & 0x3)) return OS_STACK_ALIGN_ERR;

    int reg_num = stackSize / REG_SIZE;

    // Поиск свободного слота для потока
    int threadId = 0;
    for(; threadId < MAX_THREADS; threadId++)
        if(tinyThread[threadId].state == OS_EMPTY)
            break;
    if(threadId == MAX_THREADS) return OS_TOO_MANY_THREADS_ERR;

    // Инициализация стека
    for(int i = 0; i < reg_num - NUM_OF_SYSTEM_REGS; i++) stack[i] = MAGIC_SAUCE;
    stack[reg_num - 1] = (uint32_t)proc;

    tinyThread[threadId].stackPointer = stack + reg_num - NUM_OF_SYSTEM_REGS;
    tinyThread[threadId].state = OS_RUN;
    return threadId;
}

/// Переводит текущий поток в состояние ожидания на указанное количество тиков.
/// @param d Количество тиков ожидания.
void osDelay(uint32_t d){
    tinyThread[curThread].state = OS_DELAY;
    tinyThread[curThread].tim = tick + d;
    yield();
    tinyThread[curThread].state = OS_RUN;
}

/// Переводит текущий поток в состояние ожидания до указанного значения тика.
/// @param d Абсолютное значение тика, до которого нужно ждать.
void osTim(uint32_t d){
    tinyThread[curThread].state = OS_DELAY;
    tinyThread[curThread].tim = d;
    yield();
    tinyThread[curThread].state = OS_RUN;
}

/// Ожидает, пока указанная переменная не будет соответствовать заданному условию.
/// @param p Указатель на переменную для проверки.
/// @param mask Маска для применения к переменной.
/// @param match Ожидаемое значение после применения маски.
void osWaitMatch(uint32_t* p, uint32_t mask, uint32_t match){
    tinyThread[curThread].state = OS_WAIT_MATCH;
    tinyThread[curThread].uPtr = p;
    tinyThread[curThread].mask = mask;
    tinyThread[curThread].match = match & mask;
    yield();
    tinyThread[curThread].state = OS_RUN;
}

/// Ожидает, пока указанная переменная не будет соответствовать заданному условию
/// или пока не истечет указанный таймаут.
/// @param p Указатель на переменную для проверки.
/// @param mask Маска для применения к переменной.
/// @param match Ожидаемое значение после применения маски.
/// @param d Количество тиков для таймаута.
/// @return MATCH_CONDITION, если условие выполнено, или TIMEOUT_CONDITION, если произошел таймаут.
WaitResult osWaitMatchOrDelay(uint32_t* p, uint32_t mask, uint32_t match, uint32_t d){
    tinyThread[curThread].state = OS_WAIT_MATCH_OR_DELAY;
    tinyThread[curThread].uPtr = p;
    tinyThread[curThread].mask = mask;
    tinyThread[curThread].match = match & mask;
    tinyThread[curThread].tim = tick + d;
    yield();
    tinyThread[curThread].state = OS_RUN;

    // Проверяем причину выхода
    if ((*p & mask) == (match & mask)) {
        return MATCH_CONDITION;
    } else {
        return TIMEOUT_CONDITION;
    }
}

/// Переводит поток в активное состояние.
/// @param t Номер потока.
void osThreadRun(uint8_t t){
    if(t >= MAX_THREADS) return;
    if(tinyThread[t].state == OS_EMPTY) return;
    tinyThread[t].state = OS_RUN;
}

/// Переводит поток в состояние ожидания.
/// @param t Номер потока.
void osThreadStop(uint8_t t){
    if(t >= MAX_THREADS) return;
    if(tinyThread[t].state == OS_EMPTY) return;
    tinyThread[t].state = OS_SLEEP;
}

/// Заблокирует мьютекс.
/// @param m Указатель на мьютекс.
void mutexLock(mutex_t* m) {
    __disable_irq();
    if (*m) {
        __enable_irq();
        osWaitMatch((uint32_t*)m, 1, 0);
        __disable_irq();
    }
    *m = 1;
    __enable_irq();
}

/// Заблокирует мьютекс с таймаутом.
/// @param m Указатель на мьютекс.
/// @param timeout Количество тиков для таймаута.
/// @return MATCH_CONDITION, если мьютекс заблокирован, TIMEOUT_CONDITION, если произошел таймаут.
WaitResult mutexLockWithTimeout(mutex_t* m, uint32_t timeout) {
    __disable_irq();
    if (*m) {
        // Мьютекс свободен, захватываем его
        *m = 1;
        __enable_irq();
        return MATCH_CONDITION;
    }
    __enable_irq();

    // Мьютекс занят, ждем его освобождения с таймаутом
    WaitResult result = osWaitMatchOrDelay((uint32_t*)m, 1, 0, timeout);

    if (result == MATCH_CONDITION) {
        // Мьютекс освободился, захватываем его
        __disable_irq();
        *m = 1;
        __enable_irq();
    }

    return result;
}

/// Проверяет состояние мьютекса.
/// @param m Указатель на мьютекс.
/// @return 1, если мьютекс занят, 0, если свободен.
int mutexIsLocked(mutex_t* m){
    int isLocked = 0;
    __disable_irq();
    if (*m) {
        isLocked = 1;
    }
    __enable_irq();
    return isLocked;
}

/// Освобождает мьютекс.
/// @param m Указатель на мьютекс.
void mutexUnlock(mutex_t* m){
    __disable_irq();
    *m = 0;
    __enable_irq();
}
