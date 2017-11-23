/*#define CPU_LOGGING*/

#include <stdio.h>
#include "../include/cpu_flags.h"
#include "../include/cpu_internal.h"
#include "../include/cpu_logging.h"
#include "../include/log.h"
#include "../include/ppu_internal.h"

#define LOG_INSTRUCTION_1() LOG_CPU(20, "%04X  %02X       %s",     \
    cpu.PC, opcode, cpu_names_table[opcode]);
#define LOG_INSTRUCTION_2() LOG_CPU(20, "%04X  %02X %02X    %s",   \
    cpu.PC, opcode, mem_get(cpu.PC + 1), cpu_names_table[opcode]);
#define LOG_INSTRUCTION_3() LOG_CPU(20, "%04X  %02X %02X %02X %s", \
    cpu.PC, opcode, mem_get(cpu.PC + 1), mem_get(cpu.PC + 2),      \
    cpu_names_table[opcode]);

#define ARGUMENT    mem_get   (cpu.PC + 1)
#define ARGUMENT_16 mem_get_16(cpu.PC + 1)

static Function cpu_logging_table[256];
static char *cpu_names_table[256];
static word opcode;

inline void cpu_log_set_function(byte opcode, Function function) {
    cpu_logging_table[opcode] = function;
}

inline void cpu_log_set_name(byte opcode, char *name) {
    cpu_names_table[opcode] = name;
}

inline void cpu_log_absolute(void) {
    word address = ARGUMENT_16;

    LOG_INSTRUCTION_3();
    if (address >= 0x2000 && address < 0x4020)
         LOG_CPU(28, "$%04X = %02X", address, 0xFF);
    else LOG_CPU(28, "$%04X = %02X", address, mem_get(address));
}

inline void cpu_log_absolute_jump(void) {
    LOG_INSTRUCTION_3();
    LOG_CPU(28, "$%04X", ARGUMENT_16);
}

inline void cpu_log_absolute_x(void) {
    word pointer = ARGUMENT_16;
    word address = address + cpu.X;

    LOG_INSTRUCTION_3();
    LOG_CPU(28, "$%04X,X @ %04X = %02X", pointer, address, mem_get(address));
}

inline void cpu_log_absolute_y(void) {
    word pointer = ARGUMENT_16;
    word address = address + cpu.Y;

    LOG_INSTRUCTION_3();
    LOG_CPU(28, "$%04X,Y @ %04X = %02X", pointer, address, mem_get(address));
}

inline void cpu_log_accumulator(void) {
    LOG_INSTRUCTION_1();
    LOG_CPU(28, "A");
}

inline void cpu_log_immediate(void) {
    LOG_INSTRUCTION_2();
    LOG_CPU(28, "#$%02X", ARGUMENT);
}

inline void cpu_log_implied(void) {
    LOG_INSTRUCTION_1();
    LOG_CPU(28, "");
}

inline void cpu_log_indirect(void) {
    word pointer = ARGUMENT_16;
    byte lo = mem_get(pointer), hi;
    if (pointer & 0xFF == 0xFF)
         hi = mem_get(pointer - 0xFF);
    else hi = mem_get(pointer + 1);
    word address = (hi << 8) | lo;

    LOG_INSTRUCTION_3();
    LOG_CPU(28, "($%04X) = %04X", pointer, address);
}

inline void cpu_log_indirect_x(void) {
    byte pointer = ARGUMENT;
    byte lo = cpu.ram[(pointer + cpu.X) & 0xFF];
    byte hi = cpu.ram[(pointer + cpu.X + 1) & 0xFF];
    word address = (hi << 8) | lo;

    LOG_INSTRUCTION_2();
    LOG_CPU(28, "($%02X,X) @ %02X = %04X = %02X",
        pointer, (pointer + cpu.X) & 0xFF, address, mem_get(address));
}

inline void cpu_log_indirect_y(void) {
    byte pointer = ARGUMENT;
    byte lo = cpu.ram[pointer];
    byte hi = cpu.ram[(pointer + 1) & 0xFF];
    word address = ((hi << 8) | lo) + cpu.Y;

    LOG_INSTRUCTION_2();
    LOG_CPU(28, "($%02X),Y = %04X @ %04X = %02X",
        pointer, (hi << 8) | lo, address, mem_get(address));
}

inline void cpu_log_relative(void) {
    LOG_INSTRUCTION_2();
    LOG_CPU(28, "$%04X", (word) (cpu.PC + 2 + (int8_t) ARGUMENT));
}

inline void cpu_log_zero_page(void) {
    byte address = ARGUMENT;
    LOG_INSTRUCTION_2();
    LOG_CPU(28, "$%02X = %02X", address, mem_get(address));
}

inline void cpu_log_zero_page_x(void) {
    byte pointer = ARGUMENT;
    byte address = (pointer + cpu.X) & 0xFF;

    LOG_INSTRUCTION_2();
    LOG_CPU(28, "$%02X,X @ %02X = %02X", pointer, address, mem_get(address));
}

inline void cpu_log_zero_page_y(void) {
    byte pointer = ARGUMENT;
    byte address = (pointer + cpu.Y) & 0xFF;

    LOG_INSTRUCTION_2();
    LOG_CPU(28, "$%02X,Y @ %02X = %02X", pointer, address, mem_get(address));
}

inline void cpu_log_operation(void) {
    opcode = mem_get(cpu.PC);
    (*cpu_logging_table[opcode])();
    LOG_CPU(33, "A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%3d SL:%d\n",
        cpu.A, cpu.X, cpu.Y, flg_get_status(false), cpu.S, ppu.dot,
        ppu.scanline);
}
