#include <windows.h>
#include <stdio.h>
#include "cpu.h"
#include "assembler.h"


extern char *MEMORY;

int main(int argc, const char *argv[]) {
    assemble(L"test.asm");
}