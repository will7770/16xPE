#pragma once

#include <stdint.h>


typedef struct {
    unsigned short PC;
    unsigned short reg_file[7];
} CPU;


typedef struct {
    CPU cpu;
    char *memory;
} Emulator;