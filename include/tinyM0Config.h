#pragma once
#include "tinyM0Core.h"

uint8_t proc1Stack[256] = {0x55};
uint8_t proc2Stack[256] = {0x55};
void proc1(void);
void proc2(void);

tinyThreadConfig_t tinyThreadConfig[2] = {
    {
        .proc = proc1,
        .stack = proc1Stack,
    },
    {
        .proc = proc2,
        .stack = proc2Stack,
    }
};
