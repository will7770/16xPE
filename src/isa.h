#pragma once
#include <stdint.h>


typedef enum {
    ADDI,
    ADD,

    INSTR_END,
} INSTR;


typedef void (*exec_func)(unsigned short PC);