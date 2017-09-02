#include <stdlib.h>
#include "../../include/cartridge.h"
#include "../../include/cpu.h"
#include "../../include/cpu_flags.h"
#include "../../include/cpu_internal.h"
#include "../../include/memory.h"
#include "../../include/mmc.h"
#include "../include/cpu_tests.h"
#include "../include/test_unit.h"

/* -----------------------------------------------------------------
 * Setup for tests.
 * -------------------------------------------------------------- */

static void setup(void) {
    Cartridge *cartridge = malloc(sizeof(Cartridge));
    cartridge->prg_rom = malloc(PRG_BANK_SIZE);
    cartridge->mapper = 0;
    cartridge->prg_banks = 1;

    for (int i = 0; i < PRG_BANK_SIZE; i++) {
        cartridge->prg_rom[i] = 0x00;
    }

    /* Set IQR/BRK vector to 0x1234. */
    cartridge->prg_rom[PRG_BANK_SIZE - 2] = 0x34;
    cartridge->prg_rom[PRG_BANK_SIZE - 1] = 0x12;

    mmc_init(cartridge);
    cpu_init();
}

/* -----------------------------------------------------------------
 * Help functions for loading instructions.
 * -------------------------------------------------------------- */

static const word INSTRUCTION_ADDRESS = 0x0200;

typedef bool (*Bool)(void);
typedef void (*Function)(void);
typedef void (*LoadOperand)(word, byte, byte, byte);
typedef void (*LoadAddress)(word, byte, word);

static inline void load_absolute(word pc, byte op, byte arg, byte acc) {
    cpu_init();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, 0x34);
    mem_write(pc + 2, 0x12);
    mem_write(0x1234, arg);
    cpu.A = acc;
}

static inline void load_absolute_addr(word pc, byte op, word address) {
    cpu_init();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, address & 0xFF);
    mem_write(pc + 2, address >> 8);
}

static inline void load_absolute_x(word pc, byte op, byte arg, byte acc) {
    cpu_init();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, 0x00);
    mem_write(pc + 2, 0x12);
    mem_write(0x1234, arg);
    cpu.A = acc;
    cpu.X = 0x34;
}

static inline void load_absolute_x_addr(word pc, byte op, word address) {
    cpu_init();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, address & 0xFF);
    mem_write(pc + 2, address >> 8);
    cpu.X = 0x00;
}

static inline void load_absolute_x_page(word pc, byte op, byte arg, byte acc) {
    cpu_init();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, 0xFF);
    mem_write(pc + 2, 0x12);
    mem_write(0x1300, arg);
    cpu.A = acc;
    cpu.X = 0x01;
}

static inline void load_absolute_y(word pc, byte op, byte arg, byte acc) {
    cpu_init();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, 0x00);
    mem_write(pc + 2, 0x12);
    mem_write(0x1234, arg);
    cpu.A = acc;
    cpu.Y = 0x34;
}

static inline void load_absolute_y_addr(word pc, byte op, word address) {
    cpu_init();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, address & 0xFF);
    mem_write(pc + 2, address >> 8);
    cpu.Y = 0x00;
}

static inline void load_absolute_y_page(word pc, byte op, byte arg, byte acc) {
    cpu_init();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, 0xFF);
    mem_write(pc + 2, 0x12);
    mem_write(0x1300, arg);
    cpu.A = acc;
    cpu.Y = 0x01;
}

static inline void load_accumulator(word pc, byte op, byte acc) {
    cpu_init();
    cpu.PC = pc;
    mem_write(pc, op);
    cpu.A = acc;
}

static inline void load_immediate(word pc, byte op, byte arg, byte acc) {
    cpu_init();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, arg);
    cpu.A = acc;
}

static inline void load_implied(word pc, byte op) {
    cpu_init();
    cpu.PC = pc;
    mem_write(pc, op);
}

static inline void load_indirect_addr(word pc, byte op, word address) {
    cpu_init();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, 0x34);
    mem_write(pc + 2, 0x12);
    mem_write(0x1234, address & 0xFF);
    mem_write(0x1235, address >> 8);
}

static inline void load_indirect_x(word pc, byte op, byte arg, byte acc) {
    cpu_init();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, 0xA0);
    mem_write(0x00AB, 0x34);
    mem_write(0x00AC, 0x12);
    mem_write(0x1234, arg);
    cpu.A = acc;
    cpu.X = 0x0B;
}

static inline void load_indirect_x_addr(word pc, byte op, word address) {
    cpu_init();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, 0xA0);
    mem_write(0x00AB, address & 0xFF);
    mem_write(0x00AC, address >> 8);
    cpu.X = 0x0B;
}

static inline void load_indirect_y(word pc, byte op, byte arg, byte acc) {
    cpu_init();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, 0xAB);
    mem_write(0x00AB, 0x30);
    mem_write(0x00AC, 0x12);
    mem_write(0x1234, arg);
    cpu.A = acc;
    cpu.Y = 0x04;
}

static inline void load_indirect_y_addr(word pc, byte op, word address) {
    cpu_init();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, 0xAB);
    mem_write(0x00AB, address & 0xFF);
    mem_write(0x00AC, address >> 8);
    cpu.Y = 0x00;
}

static inline void load_indirect_y_page(word pc, byte op, byte arg, byte acc) {
    cpu_init();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, 0xAB);
    mem_write(0x00AB, 0xFF);
    mem_write(0x00AC, 0x12);
    mem_write(0x1300, arg);
    cpu.A = acc;
    cpu.Y = 0x01;
}

static inline void load_relative(word pc, byte op, byte arg) {
    load_immediate(pc, op, arg, 0x00);
}

static inline void load_zero_page(word pc, byte op, byte arg, byte acc) {
    cpu_init();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, 0xAB);
    mem_write(0x00AB, arg);
    cpu.A = acc;
}

static inline void load_zero_page_addr(word pc, byte op, word address) {
    cpu_init();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, address & 0xFF);
}

static inline void load_zero_page_x(word pc, byte op, byte arg, byte acc) {
    cpu_init();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, 0xA0);
    mem_write(0x00AB, arg);
    cpu.A = acc;
    cpu.X = 0x0B;
}

static inline void load_zero_page_xy_addr(word pc, byte op, word address) {
    cpu_init();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, address & 0xFF);
    cpu.X = 0x00;
    cpu.Y = 0x00;
}

static inline void load_zero_page_y(word pc, byte op, byte arg, byte acc) {
    cpu_init();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, 0xA0);
    mem_write(0x00AB, arg);
    cpu.A = acc;
    cpu.Y = 0x0B;
}

/* -----------------------------------------------------------------
 * CPU instruction tests.
 * -------------------------------------------------------------- */

static char *test_adc(byte opcode, int cycles, char *msg, LoadOperand load) {
    /* No flags set, add with carry. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x0F, 0x05);
    flg_set_C();
    cpu_cycle(cycles);
    ASSERT(msg, cpu_get_wait_ticks() == 0);
    ASSERT(msg, cpu.A == 0x15);
    ASSERT(msg, !flg_is_C());
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, !flg_is_V());
    ASSERT(msg, !flg_is_N());

    /* Zero and carry flag set, add without carry. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0xFF, 0x01);
    flg_clear_C();
    cpu_cycle(cycles);
    ASSERT(msg, cpu.A == 0x00);
    ASSERT(msg,  flg_is_C());
    ASSERT(msg,  flg_is_Z());
    ASSERT(msg, !flg_is_V());
    ASSERT(msg, !flg_is_N());

    /* Zero and carry flag set, add with carry. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0xFF, 0x00);
    flg_set_C();
    cpu_cycle(cycles);
    ASSERT(msg, cpu.A == 0x00);
    ASSERT(msg,  flg_is_C());
    ASSERT(msg,  flg_is_Z());
    ASSERT(msg, !flg_is_V());
    ASSERT(msg, !flg_is_N());

    /* Negative and overflow flag set, add without carry. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x40, 0x40);
    flg_clear_C();
    cpu_cycle(cycles);
    ASSERT(msg, cpu.A == 0x80);
    ASSERT(msg, !flg_is_C());
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg,  flg_is_V());
    ASSERT(msg,  flg_is_N());

    /* Carry and overflow flag set, add with carry. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x80, 0x80);
    flg_set_C();
    cpu_cycle(cycles);
    ASSERT(msg, cpu.A == 0x01);
    ASSERT(msg,  flg_is_C());
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg,  flg_is_V());
    ASSERT(msg, !flg_is_N());

    /* Overflow caused by carry. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x00, 0x7F);
    flg_set_C();
    cpu_cycle(cycles);
    ASSERT(msg, cpu.A == 0x80);
    ASSERT(msg, !flg_is_C());
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg,  flg_is_V());
    ASSERT(msg,  flg_is_N());
    
    return 0;
}

static char *test_and(byte opcode, int cycles, char *msg, LoadOperand load) {
    /* No flags set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x77, 0xBB);
    cpu_cycle(cycles);
    ASSERT(msg, cpu_get_wait_ticks() == 0);
    ASSERT(msg, cpu.A == 0x33);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Zero flag set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x88, 0x33);
    cpu_cycle(cycles);
    ASSERT(msg, cpu.A == 0x00);
    ASSERT(msg, flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Negative flag set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0xBB, 0xFF);
    cpu_cycle(cycles);
    ASSERT(msg, cpu.A == 0xBB);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, flg_is_N());

    return 0;
}

static char *test_op_0A(void) { /* ASL, Accumulator. */
    /* No flags set. */
    load_accumulator(INSTRUCTION_ADDRESS, 0x0A, 0x03);
    cpu_cycle(2);
    ASSERT("ASL, Accumulator (0x0A)", cpu_get_wait_ticks() == 0);
    ASSERT("ASL, Accumulator (0x0A)", cpu.A == 0x06);
    ASSERT("ASL, Accumulator (0x0A)", !flg_is_C());
    ASSERT("ASL, Accumulator (0x0A)", !flg_is_Z());
    ASSERT("ASL, Accumulator (0x0A)", !flg_is_N());

    /* Carry and zero flag set. */
    load_accumulator(INSTRUCTION_ADDRESS, 0x0A, 0x80);
    cpu_cycle(2);
    ASSERT("ASL, Accumulator (0x0A)", cpu.A == 0x00);
    ASSERT("ASL, Accumulator (0x0A)",  flg_is_C());
    ASSERT("ASL, Accumulator (0x0A)",  flg_is_Z());
    ASSERT("ASL, Accumulator (0x0A)", !flg_is_N());

    /* Negative flag set. */
    load_accumulator(INSTRUCTION_ADDRESS, 0x0A, 0x7F);
    cpu_cycle(2);
    ASSERT("ASL, Accumulator (0x0A)", cpu.A == 0xFE);
    ASSERT("ASL, Accumulator (0x0A)", !flg_is_C());
    ASSERT("ASL, Accumulator (0x0A)", !flg_is_Z());
    ASSERT("ASL, Accumulator (0x0A)",  flg_is_N());

    return 0;
}

static char *test_asl(byte opcode, int cycles, char *msg, LoadAddress load) {
    /* No flags set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x00AB);
    mem_write(0x00AB, 0x03);
    cpu_cycle(cycles);
    ASSERT(msg, cpu_get_wait_ticks() == 0);
    ASSERT(msg, mem_read(0x00AB) == 0x06);
    ASSERT(msg, !flg_is_C());
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Carry and zero flag set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x00AB);
    mem_write(0x00AB, 0x80);
    cpu_cycle(cycles);
    ASSERT(msg, mem_read(0x00AB) == 0x00);
    ASSERT(msg,  flg_is_C());
    ASSERT(msg,  flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Negative flag set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x00AB);
    mem_write(0x00AB, 0x7F);
    cpu_cycle(cycles);
    ASSERT(msg, mem_read(0x00AB) == 0xFE);
    ASSERT(msg, !flg_is_C());
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg,  flg_is_N());

    return 0;
}

static char *test_brc(byte opcode, char *msg, Function set, Function clear) {
    /* Branching fails. */
    load_relative(INSTRUCTION_ADDRESS, opcode, 0x75);
    (*clear)();
    cpu_cycle(2);
    ASSERT(msg, cpu_get_wait_ticks() == 0);
    ASSERT(msg, cpu.PC == INSTRUCTION_ADDRESS + 2);

    /* Branching succeeds, positive offset, no page crossed. */
    load_relative(INSTRUCTION_ADDRESS + 0xFE, opcode, 0x77);
    (*set)();
    cpu_cycle(3);
    ASSERT(msg, cpu_get_wait_ticks() == 0);
    ASSERT(msg, cpu.PC == INSTRUCTION_ADDRESS + 0x177);

    /* Branching succeeds, negative offset, no page crossed. */
    load_relative(INSTRUCTION_ADDRESS + 0x75, opcode, 0x89);
    (*set)();
    cpu_cycle(3);
    ASSERT(msg, cpu_get_wait_ticks() == 0);
    ASSERT(msg, cpu.PC == INSTRUCTION_ADDRESS);

    /* Branching succeeds, positive offset, page crossed. */
    load_relative(INSTRUCTION_ADDRESS + 0xFD, opcode, 0x01);
    (*set)();
    cpu_cycle(4);
    ASSERT(msg, cpu_get_wait_ticks() == 0);
    ASSERT(msg, cpu.PC == INSTRUCTION_ADDRESS + 0x100);

    /* Branching succeeds, negative offset, page crossed. */
    load_relative(INSTRUCTION_ADDRESS, opcode, 0xFD);
    (*set)();
    cpu_cycle(4);
    ASSERT(msg, cpu_get_wait_ticks() == 0);
    ASSERT(msg, cpu.PC == INSTRUCTION_ADDRESS - 1);

    return 0;
}

static char *test_bit(byte opcode, int cycles, char *msg, LoadOperand load) {
    /* No flags set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0xFF, 0x03);
    cpu_cycle(cycles);
    ASSERT(msg, cpu_get_wait_ticks() == 0);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, !flg_is_V());
    ASSERT(msg, !flg_is_N());

    /* Zero flag set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x71, 0x88);
    cpu_cycle(cycles);
    ASSERT(msg,  flg_is_Z());
    ASSERT(msg, !flg_is_V());
    ASSERT(msg, !flg_is_N());

    /* Overflow flag set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x71, 0x40);
    cpu_cycle(cycles);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg,  flg_is_V());
    ASSERT(msg, !flg_is_N());

    /* Negative flag set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0xAA, 0x80);
    cpu_cycle(cycles);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, !flg_is_V());
    ASSERT(msg,  flg_is_N());

    return 0;
}

static char *test_op_00_40(void) { /* BRK and RTI. */
    load_implied(INSTRUCTION_ADDRESS, 0x00);
    cpu.S = 0xFF;
    flg_reset();
    flg_set_V();
    flg_set_Z();
    cpu_cycle(7);
    ASSERT("BRK, Implied (0x00)", cpu_get_wait_ticks() == 0);
    ASSERT("BRK, Implied (0x00)", cpu.PC == 0x1234);
    ASSERT("BRK, Implied (0x00)", mem_read_16(0x1FE) == INSTRUCTION_ADDRESS + 2);
    ASSERT("BRK, Implied (0x00)", mem_read(0x1FD) == 0x72);
    ASSERT("BRK, Implied (0x00)", cpu.S == 0xFC);

    mem_write(0x1234, 0x40);
    flg_clear_Z();
    flg_clear_V();
    flg_set_C();
    flg_set_N();
    cpu_cycle(6);
    ASSERT("RTI, Implied (0x40)", cpu_get_wait_ticks() == 0);
    ASSERT("RTI, Implied (0x40)", cpu.PC == INSTRUCTION_ADDRESS + 2);
    ASSERT("RTI, Implied (0x40)", cpu.S == 0xFF);
    ASSERT("RTI, Implied (0x40)", !flg_is_C());
    ASSERT("RTI, Implied (0x40)",  flg_is_Z());
    ASSERT("RTI, Implied (0x40)", !flg_is_I());
    ASSERT("RTI, Implied (0x40)", !flg_is_D());
    ASSERT("RTI, Implied (0x40)",  flg_is_V());
    ASSERT("RTI, Implied (0x40)", !flg_is_N());

    return 0;
}

static char *test_clr(byte opcode, char *msg, Function set, Bool check) {
    load_implied(INSTRUCTION_ADDRESS, opcode);
    (*set)();
    cpu_cycle(2);
    ASSERT(msg, cpu_get_wait_ticks() == 0);
    ASSERT(msg, !(*check)());

    return 0;
}

static char *test_cmp(byte opcode, int cycles, char *msg, LoadOperand load, byte *reg) {
    /* Carry flag set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x76, 0x77);
    *reg = 0x77;
    cpu_cycle(cycles);
    ASSERT(msg, cpu_get_wait_ticks() == 0);
    ASSERT(msg,  flg_is_C());
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Zero and carry flag set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0xAB, 0xAB);
    *reg = 0xAB;
    cpu_cycle(cycles);
    ASSERT(msg,  flg_is_C());
    ASSERT(msg,  flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Negative flag set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x77, 0x76);
    *reg = 0x76;
    cpu_cycle(cycles);
    ASSERT(msg, !flg_is_C());
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg,  flg_is_N());

    return 0;
}

static char *test_dec(byte opcode, int cycles, char *msg, LoadAddress load) {
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x00AB);
    mem_write(0x00AB, 0x77);
    cpu_cycle(cycles);
    ASSERT(msg, cpu_get_wait_ticks() == 0);
    ASSERT(msg, mem_read(0x00AB) == 0x76);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Flag Z is set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x00AB);
    mem_write(0x00AB, 0x01);
    cpu_cycle(cycles);
    ASSERT(msg, mem_read(0x00AB) == 0x00);
    ASSERT(msg, flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Flag N is set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x00AB);
    mem_write(0x00AB, 0x00);
    cpu_cycle(cycles);
    ASSERT(msg, mem_read(0x00AB) == 0xFF);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, flg_is_N());

    return 0;
}

static char *test_dex(byte opcode, char *msg, byte *reg) {
    load_implied(INSTRUCTION_ADDRESS, opcode);
    *reg = 0x77;
    cpu_cycle(2);
    ASSERT(msg, cpu_get_wait_ticks() == 0);
    ASSERT(msg, *reg == 0x76);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Flag Z is set. */
    load_implied(INSTRUCTION_ADDRESS, opcode);
    *reg = 0x01;
    cpu_cycle(2);
    ASSERT(msg, *reg == 0x00);
    ASSERT(msg, flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Flag N is set. */
    load_implied(INSTRUCTION_ADDRESS, opcode);
    *reg = 0x00;
    cpu_cycle(2);
    ASSERT(msg, *reg == 0xFF);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, flg_is_N());

    return 0;
}

static char *test_eor(byte opcode, int cycles, char *msg, LoadOperand load) {
    /* No flags set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x03, 0x09);
    cpu_cycle(cycles);
    ASSERT(msg, cpu_get_wait_ticks() == 0);
    ASSERT(msg, cpu.A == 0x0A);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Flag Z is set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x77, 0x77);
    cpu_cycle(cycles);
    ASSERT(msg, cpu.A == 0x00);
    ASSERT(msg, flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Flag N is set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x80, 0x7F);
    cpu_cycle(cycles);
    ASSERT(msg, cpu.A == 0xFF);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, flg_is_N());

    return 0;
}

static char *test_inc(byte opcode, int cycles, char *msg, LoadAddress load) {
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x00AB);
    mem_write(0x00AB, 0x77);
    cpu_cycle(cycles);
    ASSERT(msg, cpu_get_wait_ticks() == 0);
    ASSERT(msg, mem_read(0x00AB) == 0x78);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Flag Z is set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x00AB);
    mem_write(0x00AB, 0xFF);
    cpu_cycle(cycles);
    ASSERT(msg, mem_read(0x00AB) == 0x00);
    ASSERT(msg, flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Flag N is set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x00AB);
    mem_write(0x00AB, 0xFE);
    cpu_cycle(cycles);
    ASSERT(msg, mem_read(0x00AB) == 0xFF);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, flg_is_N());

    return 0;
}

static char *test_inx(byte opcode, char *msg, byte *reg) {
    load_implied(INSTRUCTION_ADDRESS, opcode);
    *reg = 0x77;
    cpu_cycle(2);
    ASSERT(msg, cpu_get_wait_ticks() == 0);
    ASSERT(msg, *reg == 0x78);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Flag Z is set. */
    load_implied(INSTRUCTION_ADDRESS, opcode);
    *reg = 0xFF;
    cpu_cycle(2);
    ASSERT(msg, *reg == 0x00);
    ASSERT(msg, flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Flag N is set. */
    load_implied(INSTRUCTION_ADDRESS, opcode);
    *reg = 0xFE;
    cpu_cycle(2);
    ASSERT(msg, *reg == 0xFF);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, flg_is_N());

    return 0;
}

static char *test_op_4C_6C() {
    load_absolute_addr(INSTRUCTION_ADDRESS, 0x4C, 0x1234);
    cpu_cycle(3);
    ASSERT("JMP, Absolute (0x4C)", cpu_get_wait_ticks() == 0);
    ASSERT("JMP, Absolute (0x4C)", cpu.PC == 0x1234);

    load_absolute_addr(INSTRUCTION_ADDRESS, 0x6C, 0x03FE);
    mem_write(0x03FE, 0x34);
    mem_write(0x03FF, 0x12);
    cpu_cycle(5);
    ASSERT("JMP, Indirect (0x6C)", cpu_get_wait_ticks() == 0);
    ASSERT("JMP, Indirect (0x6C)", cpu.PC == 0x1234);

    /* JMP Indirect hardware bug. */
    load_absolute_addr(INSTRUCTION_ADDRESS, 0x6C, 0x03FF);
    mem_write(0x03FF, 0x34);
    mem_write(0x0300, 0x12);
    cpu_cycle(5);
    ASSERT("JMP, Indirect (0x6C)", cpu.PC == 0x1234);

    return 0;
}

static char *test_op_20_60(void) { /* JSR and RTS. */
    load_absolute_addr(INSTRUCTION_ADDRESS, 0x20, 0x1234);
    cpu.S = 0xFF;
    cpu_cycle(6);
    ASSERT("JSR, Absolute (0x20)", cpu_get_wait_ticks() == 0);
    ASSERT("JSR, Absolute (0x20)", mem_read_16(0x1FE) == INSTRUCTION_ADDRESS + 3);
    ASSERT("JSR, Absolute (0x20)", cpu.PC == 0x1234);
    ASSERT("JSR, Absolute (0x20)", cpu.S == 0xFD);

    mem_write(0x1234, 0x60);
    cpu_cycle(6);
    ASSERT("RTS, Implied (0x60)", cpu_get_wait_ticks() == 0);
    ASSERT("RTS, Implied (0x60)", cpu.PC == INSTRUCTION_ADDRESS + 3);
    ASSERT("RTS, Implied (0x60)", cpu.S == 0xFF);

    return 0;
}

static char *test_lda(byte opcode, int cycles, char *msg, LoadOperand load, byte *reg) {
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x77, 0x00);
    *reg = 0x00;
    cpu_cycle(cycles);
    ASSERT(msg, cpu_get_wait_ticks() == 0);
    ASSERT(msg, *reg = 0x77);

    return 0;
}

static char *test_op_4A(void) { /* LSR, Accumulator. */
    /* No flags set. */
    load_accumulator(INSTRUCTION_ADDRESS, 0x4A, 0x82);
    cpu_cycle(2);
    ASSERT("LSR, Accumulator (0x4A)", cpu_get_wait_ticks() == 0);
    ASSERT("LSR, Accumulator (0x4A)", cpu.A == 0x41);
    ASSERT("LSR, Accumulator (0x4A)", !flg_is_C());
    ASSERT("LSR, Accumulator (0x4A)", !flg_is_Z());
    ASSERT("LSR, Accumulator (0x4A)", !flg_is_N());

    /* Zero flag set. */
    load_accumulator(INSTRUCTION_ADDRESS, 0x4A, 0x00);
    cpu_cycle(2);
    ASSERT("LSR, Accumulator (0x4A)", cpu.A == 0x00);
    ASSERT("LSR, Accumulator (0x4A)", !flg_is_C());
    ASSERT("LSR, Accumulator (0x4A)",  flg_is_Z());
    ASSERT("LSR, Accumulator (0x4A)", !flg_is_N());

    /* Carry flag set. */
    load_accumulator(INSTRUCTION_ADDRESS, 0x4A, 0x31);
    cpu_cycle(2);
    ASSERT("LSR, Accumulator (0x4A)", cpu.A == 0x18);
    ASSERT("LSR, Accumulator (0x4A)",  flg_is_C());
    ASSERT("LSR, Accumulator (0x4A)", !flg_is_Z());
    ASSERT("LSR, Accumulator (0x4A)", !flg_is_N());

    /* Zero and carry flag set. */
    load_accumulator(INSTRUCTION_ADDRESS, 0x4A, 0x01);
    cpu_cycle(2);
    ASSERT("LSR, Accumulator (0x4A)", cpu.A == 0x00);
    ASSERT("LSR, Accumulator (0x4A)",  flg_is_C());
    ASSERT("LSR, Accumulator (0x4A)",  flg_is_Z());
    ASSERT("LSR, Accumulator (0x4A)", !flg_is_N());

    return 0;
}

static char *test_lsr(byte opcode, int cycles, char *msg, LoadAddress load) {
    /* No flags set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x00AB);
    mem_write(0x00AB, 0x82);
    cpu_cycle(cycles);
    ASSERT(msg, cpu_get_wait_ticks() == 0);
    ASSERT(msg, mem_read(0x00AB) == 0x41);
    ASSERT(msg, !flg_is_C());
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Zero flag set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x00AB);
    mem_write(0x00AB, 0x00);
    cpu_cycle(cycles);
    ASSERT(msg, mem_read(0x00AB) == 0x00);
    ASSERT(msg, !flg_is_C());
    ASSERT(msg,  flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Carry flag set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x00AB);
    mem_write(0x00AB, 0x31);
    cpu_cycle(cycles);
    ASSERT(msg, mem_read(0x00AB) == 0x18);
    ASSERT(msg,  flg_is_C());
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Zero and carry flag set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x00AB);
    mem_write(0x00AB, 0x01);
    cpu_cycle(cycles);
    ASSERT(msg, mem_read(0x00AB) == 0x00);
    ASSERT(msg,  flg_is_C());
    ASSERT(msg,  flg_is_Z());
    ASSERT(msg, !flg_is_N());

    return 0;
}

static char *test_ora(byte opcode, int cycles, char *msg, LoadOperand load) {
    /* No flags set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x03, 0x08);
    cpu_cycle(cycles);
    ASSERT(msg, cpu_get_wait_ticks() == 0);
    ASSERT(msg, cpu.A == 0x0B);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Flag Z set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x00, 0x00);
    cpu_cycle(cycles);
    ASSERT(msg, cpu.A == 0x00);
    ASSERT(msg, flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Flag N set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x80, 0x03);
    cpu_cycle(cycles);
    ASSERT(msg, cpu.A == 0x83);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, flg_is_N());

    return 0;
}

static char *test_op_48(void) { /* PHA, Implied. */
    load_accumulator(INSTRUCTION_ADDRESS, 0x48, 0x33);
    cpu.S = 0xFF;
    cpu_cycle(3);
    ASSERT("PHA, Implied (0x48)", cpu_get_wait_ticks() == 0);
    ASSERT("PHA, Implied (0x48)", mem_read(0x1FF) == 0x33);
    ASSERT("PHA, Implied (0x48)", cpu.S == 0xFE);

    /* Wrap around. */
    load_accumulator(INSTRUCTION_ADDRESS, 0x48, 0x44);
    cpu.S = 0x00;
    cpu_cycle(3);
    ASSERT("PHA, Implied (0x48)", mem_read(0x100) == 0x44);
    ASSERT("PHA, Implied (0x48)", cpu.S == 0xFF);

    return 0;
}

static char *test_op_08(void) { /* PHP, Implied. */
    /* No flags set. */
    load_implied(INSTRUCTION_ADDRESS, 0x08);
    cpu.S = 0xFF;
    flg_reset();
    cpu_cycle(3);
    ASSERT("PHP, Implied (0x08)", cpu_get_wait_ticks() == 0);
    ASSERT("PHP, Implied (0x08)", mem_read(0x1FF) == 0x30);
    ASSERT("PHP, Implied (0x08)", cpu.S == 0xFE);

    /* Negative and decimal mode flag set. */
    load_implied(INSTRUCTION_ADDRESS, 0x08);
    cpu.S = 0xFF;
    flg_reset();
    flg_set_N();
    flg_set_D();
    cpu_cycle(3);
    ASSERT("PHP, Implied (0x08)", mem_read(0x1FF) == 0xB8);

    /* Overflow and interrupt disable set. */
    load_implied(INSTRUCTION_ADDRESS, 0x08);
    cpu.S = 0xFF;
    flg_reset();
    flg_set_V();
    flg_set_I();
    cpu_cycle(3);
    ASSERT("PHP, Implied (0x08)", mem_read(0x1FF) == 0x74);

    /* Zero and carry flag set. */
    load_implied(INSTRUCTION_ADDRESS, 0x08);
    cpu.S = 0xFF;
    flg_reset();
    flg_set_C();
    flg_set_Z();
    cpu_cycle(3);
    ASSERT("PHP, Implied (0x08)", mem_read(0x1FF) == 0x33);

    /* All flags set. */
    load_implied(INSTRUCTION_ADDRESS, 0x08);
    cpu.S = 0xFF;
    flg_reset();
    flg_set_C();
    flg_set_Z();
    flg_set_I();
    flg_set_D();
    flg_set_V();
    flg_set_N();
    cpu_cycle(3);
    ASSERT("PHP, Implied (0x08)", mem_read(0x1FF) == 0xFF);

    return 0;
}

static char *test_op_68(void) { /* PLA, Implied. */
    load_implied(INSTRUCTION_ADDRESS, 0x68);
    mem_write(0x134, 0x44);
    cpu.S = 0x33;
    cpu_cycle(4);
    ASSERT("PLA, Implied (0x68)", cpu_get_wait_ticks() == 0);
    ASSERT("PLA, Implied (0x68)", cpu.A == 0x44);
    ASSERT("PLA, Implied (0x68)", cpu.S == 0x34);

    /* Wrap around. */
    load_implied(INSTRUCTION_ADDRESS, 0x68);
    mem_write(0x100, 0x44);
    cpu.S = 0xFF;
    cpu_cycle(4);
    ASSERT("PLA, Implied (0x68)", cpu.A == 0x44);
    ASSERT("PLA, Implied (0x68)", cpu.S == 0x00);

    return 0;
}

static char *test_op_28(void) { /* PLP, Implied. */
    /* No flags set. */
    load_implied(INSTRUCTION_ADDRESS, 0x28);
    mem_write(0x1FF, 0x00);
    cpu.S = 0xFE;
    cpu_cycle(4);
    ASSERT("PLP, Implied (0x28)", cpu_get_wait_ticks() == 0);
    ASSERT("PLP, Implied (0x28)", !flg_is_C());
    ASSERT("PLP, Implied (0x28)", !flg_is_Z());
    ASSERT("PLP, Implied (0x28)", !flg_is_I());
    ASSERT("PLP, Implied (0x28)", !flg_is_D());
    ASSERT("PLP, Implied (0x28)", !flg_is_V());
    ASSERT("PLP, Implied (0x28)", !flg_is_N());

    /* Negative and decimal mode flags set. */
    load_implied(INSTRUCTION_ADDRESS, 0x28);
    mem_write(0x1FF, 0x88);
    cpu.S = 0xFE;
    cpu_cycle(4);
    ASSERT("PLP, Implied (0x28)", !flg_is_C());
    ASSERT("PLP, Implied (0x28)", !flg_is_Z());
    ASSERT("PLP, Implied (0x28)", !flg_is_I());
    ASSERT("PLP, Implied (0x28)",  flg_is_D());
    ASSERT("PLP, Implied (0x28)", !flg_is_V());
    ASSERT("PLP, Implied (0x28)",  flg_is_N());

    /* Overflow and interrupt disable set. */
    load_implied(INSTRUCTION_ADDRESS, 0x28);
    mem_write(0x1FF, 0x74);
    cpu.S = 0xFE;
    cpu_cycle(4);
    ASSERT("PLP, Implied (0x28)", !flg_is_C());
    ASSERT("PLP, Implied (0x28)", !flg_is_Z());
    ASSERT("PLP, Implied (0x28)",  flg_is_I());
    ASSERT("PLP, Implied (0x28)", !flg_is_D());
    ASSERT("PLP, Implied (0x28)",  flg_is_V());
    ASSERT("PLP, Implied (0x28)", !flg_is_N());

    /* Zero and carry flag set. */
    load_implied(INSTRUCTION_ADDRESS, 0x28);
    mem_write(0x1FF, 0x33);
    cpu.S = 0xFE;
    cpu_cycle(4);
    ASSERT("PLP, Implied (0x28)",  flg_is_C());
    ASSERT("PLP, Implied (0x28)",  flg_is_Z());
    ASSERT("PLP, Implied (0x28)", !flg_is_I());
    ASSERT("PLP, Implied (0x28)", !flg_is_D());
    ASSERT("PLP, Implied (0x28)", !flg_is_V());
    ASSERT("PLP, Implied (0x28)", !flg_is_N());

    /* All flags set. */
    load_implied(INSTRUCTION_ADDRESS, 0x28);
    mem_write(0x1FF, 0xFF);
    cpu.S = 0xFE;
    cpu_cycle(4);
    ASSERT("PLP, Implied (0x28)",  flg_is_C());
    ASSERT("PLP, Implied (0x28)",  flg_is_Z());
    ASSERT("PLP, Implied (0x28)",  flg_is_I());
    ASSERT("PLP, Implied (0x28)",  flg_is_D());
    ASSERT("PLP, Implied (0x28)",  flg_is_V());
    ASSERT("PLP, Implied (0x28)",  flg_is_N());

    return 0;
}

static char *test_op_2A(void) { /* ROL, Accumulator. */
    /* No flags set. */
    load_accumulator(INSTRUCTION_ADDRESS, 0x2A, 0x31);
    cpu_cycle(2);
    ASSERT("ROL, Accumulator (0x2A)", cpu_get_wait_ticks() == 0);
    ASSERT("ROL, Accumulator (0x2A)", cpu.A == 0x62);
    ASSERT("ROL, Accumulator (0x2A)", !flg_is_C());
    ASSERT("ROL, Accumulator (0x2A)", !flg_is_Z());
    ASSERT("ROL, Accumulator (0x2A)", !flg_is_N());

    /* Carry flag set. */
    load_accumulator(INSTRUCTION_ADDRESS, 0x2A, 0x88);
    cpu_cycle(2);
    ASSERT("ROL, Accumulator (0x2A)", cpu.A == 0x11);
    ASSERT("ROL, Accumulator (0x2A)",  flg_is_C());
    ASSERT("ROL, Accumulator (0x2A)", !flg_is_Z());
    ASSERT("ROL, Accumulator (0x2A)", !flg_is_N());

    /* Zero flag set. */
    load_accumulator(INSTRUCTION_ADDRESS, 0x2A, 0x00);
    cpu_cycle(2);
    ASSERT("ROL, Accumulator (0x2A)", cpu.A == 0x00);
    ASSERT("ROL, Accumulator (0x2A)", !flg_is_C());
    ASSERT("ROL, Accumulator (0x2A)",  flg_is_Z());
    ASSERT("ROL, Accumulator (0x2A)", !flg_is_N());

    /* Negative flag set. */
    load_accumulator(INSTRUCTION_ADDRESS, 0x2A, 0x78);
    cpu_cycle(2);
    ASSERT("ROL, Accumulator (0x2A)", cpu.A == 0xF0);
    ASSERT("ROL, Accumulator (0x2A)", !flg_is_C());
    ASSERT("ROL, Accumulator (0x2A)", !flg_is_Z());
    ASSERT("ROL, Accumulator (0x2A)",  flg_is_N());

    return 0;
}

static char *test_rol(byte opcode, int cycles, char *msg, LoadAddress load) {
    /* No flags set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x00AB);
    mem_write(0x00AB, 0x31);
    cpu_cycle(cycles);
    ASSERT(msg, cpu_get_wait_ticks() == 0);
    ASSERT(msg, mem_read(0x00AB) == 0x62);
    ASSERT(msg, !flg_is_C());
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Carry flag set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x00AB);
    mem_write(0x00AB, 0x88);
    cpu_cycle(cycles);
    ASSERT(msg, mem_read(0x00AB) == 0x11);
    ASSERT(msg,  flg_is_C());
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Zero flag set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x00AB);
    mem_write(0x00AB, 0x00);
    cpu_cycle(cycles);
    ASSERT(msg, mem_read(0x00AB) == 0x00);
    ASSERT(msg, !flg_is_C());
    ASSERT(msg,  flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Negative flag set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x00AB);
    mem_write(0x00AB, 0x78);
    cpu_cycle(cycles);
    ASSERT(msg, mem_read(0x00AB) == 0xF0);
    ASSERT(msg, !flg_is_C());
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg,  flg_is_N());

    return 0;
}

static char *test_op_6A(void) { /* ROR, Accumulator. */
    /* No flags set. */
    load_accumulator(INSTRUCTION_ADDRESS, 0x6A, 0x82);
    cpu_cycle(2);
    ASSERT("ROR, Accumulator (0x6A)", cpu_get_wait_ticks() == 0);
    ASSERT("ROR, Accumulator (0x6A)", cpu.A == 0x41);
    ASSERT("ROR, Accumulator (0x6A)", !flg_is_C());
    ASSERT("ROR, Accumulator (0x6A)", !flg_is_Z());
    ASSERT("ROR, Accumulator (0x6A)", !flg_is_N());

    /* Zero flag set. */
    load_accumulator(INSTRUCTION_ADDRESS, 0x6A, 0x00);
    cpu_cycle(2);
    ASSERT("ROR, Accumulator (0x6A)", cpu.A == 0x00);
    ASSERT("ROR, Accumulator (0x6A)", !flg_is_C());
    ASSERT("ROR, Accumulator (0x6A)",  flg_is_Z());
    ASSERT("ROR, Accumulator (0x6A)", !flg_is_N());

    /* Carry and negative flag set. */
    load_accumulator(INSTRUCTION_ADDRESS, 0x6A, 0x31);
    cpu_cycle(2);
    ASSERT("ROR, Accumulator (0x6A)", cpu.A == 0x98);
    ASSERT("ROR, Accumulator (0x6A)",  flg_is_C());
    ASSERT("ROR, Accumulator (0x6A)", !flg_is_Z());
    ASSERT("ROR, Accumulator (0x6A)",  flg_is_N());

    return 0;
}

static char *test_ror(byte opcode, int cycles, char *msg, LoadAddress load) {
    /* No flags set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x00AB);
    mem_write(0x00AB, 0x82);
    cpu_cycle(cycles);
    ASSERT(msg, cpu_get_wait_ticks() == 0);
    ASSERT(msg, mem_read(0x00AB) == 0x41);
    ASSERT(msg, !flg_is_C());
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Zero flag set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x00AB);
    mem_write(0x00AB, 0x00);
    cpu_cycle(cycles);
    ASSERT(msg, mem_read(0x00AB) == 0x00);
    ASSERT(msg, !flg_is_C());
    ASSERT(msg,  flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Carry and negative flag set. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x00AB);
    mem_write(0x00AB, 0x31);
    cpu_cycle(cycles);
    ASSERT(msg, mem_read(0x00AB) == 0x98);
    ASSERT(msg,  flg_is_C());
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg,  flg_is_N());

    return 0;
}

static char *test_sbc(byte opcode, int cycles, char *msg, LoadOperand load) {
    /* Carry flag set, subtract with carry. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x74, 0x77);
    flg_set_C();
    cpu_cycle(cycles);
    ASSERT(msg, cpu_get_wait_ticks() == 0);
    ASSERT(msg, cpu.A == 0x03);
    ASSERT(msg,  flg_is_C());
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, !flg_is_V());
    ASSERT(msg, !flg_is_N());

    /* Carry and zero flag set, subtract without carry. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x76, 0x77);
    flg_clear_C();
    cpu_cycle(cycles);
    ASSERT(msg, cpu.A == 0x00);
    ASSERT(msg,  flg_is_C());
    ASSERT(msg,  flg_is_Z());
    ASSERT(msg, !flg_is_V());
    ASSERT(msg, !flg_is_N());

    /* Negative flag set, substract with carry. */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x77, 0x76);
    flg_set_C();
    cpu_cycle(cycles);
    ASSERT(msg, cpu.A == 0xFF);
    ASSERT(msg, !flg_is_C());
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, !flg_is_V());
    ASSERT(msg,  flg_is_N());

    /* Overflow caused by carry (I). */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x00, 0x80);
    flg_clear_C();
    cpu_cycle(cycles);
    ASSERT(msg, cpu.A == 0x7F);
    ASSERT(msg, !flg_is_C());
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg,  flg_is_V());
    ASSERT(msg, !flg_is_N());

    /* Overflow caused by carry (II). */
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x7F, 0xFF);
    flg_clear_C();
    cpu_cycle(cycles);
    ASSERT(msg, cpu.A == 0x7F);
    ASSERT(msg, !flg_is_C());
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg,  flg_is_V());
    ASSERT(msg, !flg_is_N());

    return 0;
}

static char *test_set(byte opcode, char *msg, Function clear, Bool check) {
    load_implied(INSTRUCTION_ADDRESS, opcode);
    (*clear)();
    cpu_cycle(2);
    ASSERT(msg, cpu_get_wait_ticks() == 0);
    ASSERT(msg, (*check)());

    return 0;
}

static char *test_str(byte opcode, int cycles, char *msg, LoadAddress load, byte *store) {
    (*load)(INSTRUCTION_ADDRESS, opcode, 0x0012);
    *store = 0x77;
    cpu_cycle(cycles);
    ASSERT(msg, cpu_get_wait_ticks() == 0);
    ASSERT(msg, mem_read(0x0012) == 0x77);

    return 0;
}

static char *test_trn(byte opcode, char *msg, byte *source, byte *dest, bool test_flags) {
    load_implied(INSTRUCTION_ADDRESS, opcode);
    *source = 0x77;
    *dest = 0x00;
    cpu_cycle(2);
    ASSERT(msg, cpu_get_wait_ticks() == 0);
    ASSERT(msg, *dest == 0x77);

    if (test_flags) {
        ASSERT(msg, !flg_is_Z());
        ASSERT(msg, !flg_is_N());

        load_implied(INSTRUCTION_ADDRESS, opcode);
        *source = 0x00;
        cpu_cycle(2);
        ASSERT(msg, *dest == 0x00);
        ASSERT(msg, flg_is_Z());
        ASSERT(msg, !flg_is_N());

        load_implied(INSTRUCTION_ADDRESS, opcode);
        *source = 0x83;
        cpu_cycle(2);
        ASSERT(msg, *dest == 0x83);
        ASSERT(msg, !flg_is_Z());
        ASSERT(msg, flg_is_N());
    } else {
        load_implied(INSTRUCTION_ADDRESS, opcode);
        flg_clear_Z();
        *source = 0x00;
        cpu_cycle(2);
        ASSERT(msg, *dest == 0x00);
        ASSERT(msg, !flg_is_Z());
    }

    return 0;
}

static char *test_op_69  (void) { return test_adc(0x69, 2, "ADC, Immediate (0x69)",      load_immediate); }
static char *test_op_65  (void) { return test_adc(0x65, 3, "ADC, Zero page (0x65)",      load_zero_page); }
static char *test_op_75  (void) { return test_adc(0x75, 4, "ADC, Zero page X (0x75)",    load_zero_page_x); }
static char *test_op_6D  (void) { return test_adc(0x6D, 4, "ADC, Absolute (0x6D)",       load_absolute); }
static char *test_op_7D_1(void) { return test_adc(0x7D, 4, "ADC, Absolute X (0x7D)",     load_absolute_x); }
static char *test_op_7D_2(void) { return test_adc(0x7D, 5, "ADC, Absolute X (0x7D) (P)", load_absolute_x_page); }
static char *test_op_79_1(void) { return test_adc(0x79, 4, "ADC, Absolute Y (0x79)",     load_absolute_y); }
static char *test_op_79_2(void) { return test_adc(0x79, 5, "ADC, Absolute Y (0x79) (P)", load_absolute_y_page); }
static char *test_op_61  (void) { return test_adc(0x61, 6, "ADC, Indirect X (0x61)",     load_indirect_x); }
static char *test_op_71_1(void) { return test_adc(0x71, 5, "ADC, Indirect Y (0x71)",     load_indirect_y); }
static char *test_op_71_2(void) { return test_adc(0x71, 6, "ADC, Indirect Y (0x71) (P)", load_indirect_y_page); }

static char *test_op_29  (void) { return test_and(0x29, 2, "AND, Immediate (0x29)",      load_immediate); }
static char *test_op_25  (void) { return test_and(0x25, 3, "AND, Zero page (0x25)",      load_zero_page); }
static char *test_op_35  (void) { return test_and(0x35, 4, "AND, Zero page X (0x35)",    load_zero_page_x); }
static char *test_op_2D  (void) { return test_and(0x2D, 4, "AND, Absolute (0x2D)",       load_absolute); }
static char *test_op_3D_1(void) { return test_and(0x3D, 4, "AND, Absolute X (0x3D)",     load_absolute_x); }
static char *test_op_3D_2(void) { return test_and(0x3D, 5, "AND, Absolute X (0x3D) (P)", load_absolute_x_page); }
static char *test_op_39_1(void) { return test_and(0x39, 4, "AND, Absolute Y (0x39)",     load_absolute_y); }
static char *test_op_39_2(void) { return test_and(0x39, 5, "AND, Absolute Y (0x39) (P)", load_absolute_y_page); }
static char *test_op_21  (void) { return test_and(0x21, 6, "AND, Indirect X (0x21)",     load_indirect_x); }
static char *test_op_31_1(void) { return test_and(0x31, 5, "AND, Indirect Y (0x31)",     load_indirect_y); }
static char *test_op_31_2(void) { return test_and(0x31, 6, "AND, Indirect Y (0x31) (P)", load_indirect_y_page); }

static char *test_op_06  (void) { return test_asl(0x06, 5, "ASL, Zero page (0x06)",      load_zero_page_addr); }
static char *test_op_16  (void) { return test_asl(0x16, 6, "ASL, Zero page X (0x16)",    load_zero_page_xy_addr); }
static char *test_op_0E  (void) { return test_asl(0x0E, 6, "ASL, Absolute (0x0E)",       load_absolute_addr); }
static char *test_op_1E  (void) { return test_asl(0x1E, 7, "ASL, Absolute X (0x1E)",     load_absolute_x_addr); }

static char *test_op_24  (void) { return test_bit(0x24, 3, "BIT, Zero page (0x24)",      load_zero_page); }
static char *test_op_2C  (void) { return test_bit(0x2C, 4, "BIT, Absolute (0x2C)",       load_absolute); }

static char *test_op_B0  (void) { return test_brc(0xB0,    "BCS, Relative (0xB0)",       flg_set_C, flg_clear_C); }
static char *test_op_90  (void) { return test_brc(0x90,    "BCC, Relative (0x90)",       flg_clear_C, flg_set_C); }
static char *test_op_F0  (void) { return test_brc(0xF0,    "BEQ, Relative (0xF0)",       flg_set_Z, flg_clear_Z); }
static char *test_op_D0  (void) { return test_brc(0xD0,    "BNE, Relative (0xD0)",       flg_clear_Z, flg_set_Z); }
static char *test_op_70  (void) { return test_brc(0x70,    "BVS, Relative (0x70)",       flg_set_V, flg_clear_V); }
static char *test_op_50  (void) { return test_brc(0x50,    "BVS, Relative (0x50)",       flg_clear_V, flg_set_V); }
static char *test_op_30  (void) { return test_brc(0x30,    "BMI, Relative (0x30)",       flg_set_N, flg_clear_N); }
static char *test_op_10  (void) { return test_brc(0x10,    "BPL, Relative (0x10)",       flg_clear_N, flg_set_N); }

static char *test_op_18  (void) { return test_clr(0x18,    "CLC, Implied (0x18)",        flg_set_C, flg_is_C); }
static char *test_op_D8  (void) { return test_clr(0xD8,    "CLD, Implied (0xD8)",        flg_set_D, flg_is_D); }
static char *test_op_58  (void) { return test_clr(0x58,    "CLI, Implied (0x58)",        flg_set_I, flg_is_I); }
static char *test_op_B8  (void) { return test_clr(0xB8,    "CLV, Implied (0xB8)",        flg_set_V, flg_is_V); }

static char *test_op_C9  (void) { return test_cmp(0xC9, 2, "CMP, Immediate (0xC9)",      load_immediate, &cpu.A); }
static char *test_op_C5  (void) { return test_cmp(0xC5, 3, "CMP, Zero page (0xC5)",      load_zero_page, &cpu.A); }
static char *test_op_D5  (void) { return test_cmp(0xD5, 4, "CMP, Zero page X (0xD5)",    load_zero_page_x, &cpu.A); }
static char *test_op_CD  (void) { return test_cmp(0xCD, 4, "CMP, Absolute (0xCD)",       load_absolute, &cpu.A); }
static char *test_op_DD_1(void) { return test_cmp(0xDD, 4, "CMP, Absolute X (0xDD)",     load_absolute_x, &cpu.A); }
static char *test_op_DD_2(void) { return test_cmp(0xDD, 5, "CMP, Absolute X (0xDD) (P)", load_absolute_x_page, &cpu.A); }
static char *test_op_D9_1(void) { return test_cmp(0xD9, 4, "CMP, Absolute Y (0xD9)",     load_absolute_y, &cpu.A); }
static char *test_op_D9_2(void) { return test_cmp(0xD9, 5, "CMP, Absolute Y (0xD9) (P)", load_absolute_y_page, &cpu.A); }
static char *test_op_C1  (void) { return test_cmp(0xC1, 6, "CMP, Indirect X (0xC1)",     load_indirect_x, &cpu.A); }
static char *test_op_D1_1(void) { return test_cmp(0xD1, 5, "CMP, Indirect Y (0xD1)",     load_indirect_y, &cpu.A); }
static char *test_op_D1_2(void) { return test_cmp(0xD1, 6, "CMP, Indirect Y (0xD1) (P)", load_indirect_y_page, &cpu.A); }
static char *test_op_E0  (void) { return test_cmp(0xE0, 2, "CPX, Immediate (0xE0)",      load_immediate, &cpu.X); }
static char *test_op_E4  (void) { return test_cmp(0xE4, 3, "CPX, Zero page (0xE4)",      load_zero_page, &cpu.X); }
static char *test_op_EC  (void) { return test_cmp(0xEC, 4, "CPX, Absolute (0xEC)",       load_absolute, &cpu.X); }
static char *test_op_C0  (void) { return test_cmp(0xC0, 2, "CPY, Immediate (0xC0)",      load_immediate, &cpu.Y); }
static char *test_op_C4  (void) { return test_cmp(0xC4, 3, "CPY, Zero page (0xC4)",      load_zero_page, &cpu.Y); }
static char *test_op_CC  (void) { return test_cmp(0xCC, 4, "CPY, Absolute (0xCC)",       load_absolute, &cpu.Y); }

static char *test_op_C6  (void) { return test_dec(0xC6, 5, "DEC, Zero page (0xC6)",      load_zero_page_addr); }
static char *test_op_D6  (void) { return test_dec(0xD6, 6, "DEC, Zero page X (0xD6)",    load_zero_page_xy_addr); }
static char *test_op_CE  (void) { return test_dec(0xCE, 6, "DEC, Absolute (0xCE)",       load_absolute_addr); }
static char *test_op_DE  (void) { return test_dec(0xDE, 7, "DEC, Absolute X (0xDE)",     load_absolute_x_addr); }
static char *test_op_CA  (void) { return test_dex(0xCA,    "DEX, Implied (0xCA)",        &cpu.X); }
static char *test_op_88  (void) { return test_dex(0x88,    "DEY, Implied (0x88)",        &cpu.Y); }

static char *test_op_49  (void) { return test_eor(0x49, 2, "EOR, Immediate (0x49)",      load_immediate); }
static char *test_op_45  (void) { return test_eor(0x45, 3, "EOR, Zero page (0x45)",      load_zero_page); }
static char *test_op_55  (void) { return test_eor(0x55, 4, "EOR, Zero page X (0x55)",    load_zero_page_x); }
static char *test_op_4D  (void) { return test_eor(0x4D, 4, "EOR, Absolute (0x4D)",       load_absolute); }
static char *test_op_5D_1(void) { return test_eor(0x5D, 4, "EOR, Absolute X (0x5D)",     load_absolute_x); }
static char *test_op_5D_2(void) { return test_eor(0x5D, 5, "EOR, Absolute X (0x5D) (P)", load_absolute_x_page); }
static char *test_op_59_1(void) { return test_eor(0x59, 4, "EOR, Absolute Y (0x59)",     load_absolute_y); }
static char *test_op_59_2(void) { return test_eor(0x59, 5, "EOR, Absolute Y (0x59) (P)", load_absolute_y_page); }
static char *test_op_41  (void) { return test_eor(0x41, 6, "EOR, Indirect X (0x41)",     load_indirect_x); }
static char *test_op_51_1(void) { return test_eor(0x51, 5, "EOR, Indirect Y (0x51)",     load_indirect_y); }
static char *test_op_51_2(void) { return test_eor(0x51, 6, "EOR, Indirect Y (0x51) (P)", load_indirect_y_page); }

static char *test_op_E6  (void) { return test_inc(0xE6, 5, "INC, Zero page (0xE6)",      load_zero_page_addr); }
static char *test_op_F6  (void) { return test_inc(0xF6, 6, "INC, Zero page X (0xF6)",    load_zero_page_xy_addr); }
static char *test_op_EE  (void) { return test_inc(0xEE, 6, "INC, Absolute (0xEE)",       load_absolute_addr); }
static char *test_op_FE  (void) { return test_inc(0xFE, 7, "INC, Absolute X (0xFE)",     load_absolute_x_addr); }
static char *test_op_E8  (void) { return test_inx(0xE8,    "INX, Implied (0xE8)",        &cpu.X); }
static char *test_op_C8  (void) { return test_inx(0xC8,    "INY, Implied (0xC8)",        &cpu.Y); }

static char *test_op_A9  (void) { return test_lda(0xA9, 2, "LDA, Immediate (0xA9)",      load_immediate, &cpu.A); }
static char *test_op_A5  (void) { return test_lda(0xA5, 3, "LDA, Zero page (0xA5)",      load_zero_page, &cpu.A); }
static char *test_op_B5  (void) { return test_lda(0xB5, 4, "LDA, Zero page X (0xB5)",    load_zero_page_x, &cpu.A); }
static char *test_op_AD  (void) { return test_lda(0xAD, 4, "LDA, Absolute (0xAD)",       load_absolute, &cpu.A); }
static char *test_op_BD_1(void) { return test_lda(0xBD, 4, "LDA, Absolute X (0xBD)",     load_absolute_x, &cpu.A); }
static char *test_op_BD_2(void) { return test_lda(0xBD, 5, "LDA, Absolute X (0xBD) (P)", load_absolute_x_page, &cpu.A); }
static char *test_op_B9_1(void) { return test_lda(0xB9, 4, "LDA, Absolute Y (0xB9)",     load_absolute_y, &cpu.A); }
static char *test_op_B9_2(void) { return test_lda(0xB9, 5, "LDA, Absolute Y (0xB9) (P)", load_absolute_y_page, &cpu.A); }
static char *test_op_A1  (void) { return test_lda(0xA1, 6, "LDA, Indirect X (0xA1)",     load_indirect_x, &cpu.A); }
static char *test_op_B1_1(void) { return test_lda(0xB1, 5, "LDA, Indirect Y (0xB1)",     load_indirect_y, &cpu.A); }
static char *test_op_B1_2(void) { return test_lda(0xB1, 6, "LDA, Indirect Y (0xB1) (P)", load_indirect_y_page, &cpu.A); }
static char *test_op_A2  (void) { return test_lda(0xA2, 2, "LDX, Immediate (0xA2)",      load_immediate, &cpu.X); }
static char *test_op_A6  (void) { return test_lda(0xA6, 3, "LDX, Zero page (0xA6)",      load_zero_page, &cpu.X); }
static char *test_op_B6  (void) { return test_lda(0xB6, 4, "LDX, Zero page Y (0xB6)",    load_zero_page_y, &cpu.X); }
static char *test_op_AE  (void) { return test_lda(0xAE, 4, "LDX, Absolute (0xAE)",       load_absolute, &cpu.X); }
static char *test_op_BE_1(void) { return test_lda(0xBE, 4, "LDX, Absolute Y (0xBE)",     load_absolute_y, &cpu.X); }
static char *test_op_BE_2(void) { return test_lda(0xBE, 5, "LDX, Absolute Y (0xBE) (P)", load_absolute_y_page, &cpu.X); }
static char *test_op_A0  (void) { return test_lda(0xA0, 2, "LDY, Immediate (0xA0)",      load_immediate, &cpu.Y); }
static char *test_op_A4  (void) { return test_lda(0xA4, 3, "LDY, Zero page (0xA4)",      load_zero_page, &cpu.Y); }
static char *test_op_B4  (void) { return test_lda(0xB4, 4, "LDY, Zero page X (0xB4)",    load_zero_page_x, &cpu.Y); }
static char *test_op_AC  (void) { return test_lda(0xAC, 4, "LDY, Absolute (0xAC)",       load_absolute, &cpu.Y); }
static char *test_op_BC_1(void) { return test_lda(0xBC, 4, "LDY, Absolute X (0xBC)",     load_absolute_x, &cpu.Y); }
static char *test_op_BC_2(void) { return test_lda(0xBC, 5, "LDY, Absolute X (0xBC) (P)", load_absolute_x_page, &cpu.Y); }

static char *test_op_46  (void) { return test_lsr(0x46, 5, "LSR, Zero page (0x46)",      load_zero_page_addr); }
static char *test_op_56  (void) { return test_lsr(0x56, 6, "LSR, Zero page X (0x56)",    load_zero_page_xy_addr); }
static char *test_op_4E  (void) { return test_lsr(0x4E, 6, "LSR, Absolute (0x4E)",       load_absolute_addr); }
static char *test_op_5E  (void) { return test_lsr(0x5E, 7, "LSR, Absolute X (0x5E)",     load_absolute_x_addr); }

static char *test_op_09  (void) { return test_ora(0x09, 2, "ORA, Immediate (0x09)",      load_immediate); }
static char *test_op_05  (void) { return test_ora(0x05, 3, "ORA, Zero page (0x05)",      load_zero_page); }
static char *test_op_15  (void) { return test_ora(0x15, 4, "ORA, Zero page X (0x15)",    load_zero_page_x); }
static char *test_op_0D  (void) { return test_ora(0x0D, 4, "ORA, Absolute (0x0D)",       load_absolute); }
static char *test_op_1D_1(void) { return test_ora(0x1D, 4, "ORA, Absolute X (0x1D)",     load_absolute_x); }
static char *test_op_1D_2(void) { return test_ora(0x1D, 5, "ORA, Absolute X (0x1D) (P)", load_absolute_x_page); }
static char *test_op_19_1(void) { return test_ora(0x19, 4, "ORA, Absolute Y (0x19)",     load_absolute_y); }
static char *test_op_19_2(void) { return test_ora(0x19, 5, "ORA, Absolute Y (0x19) (P)", load_absolute_y_page); }
static char *test_op_01  (void) { return test_ora(0x01, 6, "ORA, Indirect X (0x01)",     load_indirect_x); }
static char *test_op_11_1(void) { return test_ora(0x11, 5, "ORA, Indirect Y (0x11)",     load_indirect_y); }
static char *test_op_11_2(void) { return test_ora(0x11, 6, "ORA, Indirect Y (0x11) (P)", load_indirect_y_page); }

static char *test_op_26  (void) { return test_rol(0x26, 5, "ROL, Zero page (0x26)",      load_zero_page_addr); }
static char *test_op_36  (void) { return test_rol(0x36, 6, "ROL, Zero page X. (0x36)",   load_zero_page_xy_addr); }
static char *test_op_2E  (void) { return test_rol(0x2E, 6, "ROL, Absolute (0x2E)",       load_absolute_addr); }
static char *test_op_3E  (void) { return test_rol(0x3E, 7, "ROL, Absolute X (0x3E)",     load_absolute_x_addr); }

static char *test_op_66  (void) { return test_ror(0x66, 5, "ROR, Zero page (0x66)",      load_zero_page_addr); }
static char *test_op_76  (void) { return test_ror(0x76, 6, "ROR, Zero page X. (0x76)",   load_zero_page_xy_addr); }
static char *test_op_6E  (void) { return test_ror(0x6E, 6, "ROR, Absolute (0x6E)",       load_absolute_addr); }
static char *test_op_7E  (void) { return test_ror(0x7E, 7, "ROR, Absolute X (0x7E)",     load_absolute_x_addr); }

static char *test_op_E9  (void) { return test_sbc(0xE9, 2, "SBC, Immediate (0xE9)",      load_immediate); }
static char *test_op_E5  (void) { return test_sbc(0xE5, 3, "SBC, Zero page (0xE5)",      load_zero_page); }
static char *test_op_F5  (void) { return test_sbc(0xF5, 4, "SBC, Zero page X (0xF5)",    load_zero_page_x); }
static char *test_op_ED  (void) { return test_sbc(0xED, 4, "SBC, Absolute (0xED)",       load_absolute); }
static char *test_op_FD_1(void) { return test_sbc(0xFD, 4, "SBC, Absolute X (0xFD)",     load_absolute_x); }
static char *test_op_FD_2(void) { return test_sbc(0xFD, 5, "SBC, Absolute X (0xFD) (P)", load_absolute_x_page); }
static char *test_op_F9_1(void) { return test_sbc(0xF9, 4, "SBC, Absolute Y (0xF9)",     load_absolute_y); }
static char *test_op_F9_2(void) { return test_sbc(0xF9, 5, "SBC, Absolute Y (0xF9) (P)", load_absolute_y_page); }
static char *test_op_E1  (void) { return test_sbc(0xE1, 6, "SBC, Indirect X (0xE1)",     load_indirect_x); }
static char *test_op_F1_1(void) { return test_sbc(0xF1, 5, "SBC, Indirect Y (0xF1)",     load_indirect_y); }
static char *test_op_F1_2(void) { return test_sbc(0xF1, 6, "SBC, Indirect Y (0xF1) (P)", load_indirect_y_page); }

static char *test_op_38  (void) { return test_set(0x38,    "SEC, Implied (0x38)",        flg_clear_C, flg_is_C); }
static char *test_op_F8  (void) { return test_set(0xF8,    "SED, Implied (0xF8)",        flg_clear_D, flg_is_D); }
static char *test_op_78  (void) { return test_set(0x78,    "SEI, Implied (0x78)",        flg_clear_I, flg_is_I); }

static char *test_op_85  (void) { return test_str(0x85, 3, "STA, Zero page (0x85)",      load_zero_page_addr, &cpu.A); }
static char *test_op_95  (void) { return test_str(0x95, 4, "STA, Zero page X (0x95)",    load_zero_page_xy_addr, &cpu.A); }
static char *test_op_8D  (void) { return test_str(0x8D, 4, "STA, Absolute (0x8D)",       load_absolute_addr, &cpu.A); }
static char *test_op_9D  (void) { return test_str(0x9D, 5, "STA, Absolute X (0x9D)",     load_absolute_x_addr, &cpu.A); }
static char *test_op_99  (void) { return test_str(0x99, 5, "STA, Absolute Y (0x99)",     load_absolute_y_addr, &cpu.A); }
static char *test_op_81  (void) { return test_str(0x81, 6, "STA, Indirect X (0x81)",     load_indirect_x_addr, &cpu.A); }
static char *test_op_91  (void) { return test_str(0x91, 6, "STA, Indirect Y (0x91)",     load_indirect_y_addr, &cpu.A); }
static char *test_op_86  (void) { return test_str(0x86, 3, "STX, Zero page (0x86)",      load_zero_page_addr, &cpu.X); }
static char *test_op_96  (void) { return test_str(0x96, 4, "STX, Zero page Y (0x96)",    load_zero_page_xy_addr, &cpu.X); }
static char *test_op_8E  (void) { return test_str(0x8E, 4, "STX, Absolute (0x8E)",       load_absolute_addr, &cpu.X); }
static char *test_op_84  (void) { return test_str(0x84, 3, "STY, Zero page (0x84)",      load_zero_page_addr, &cpu.Y); }
static char *test_op_94  (void) { return test_str(0x94, 4, "STY, Zero page X (0x94)",    load_zero_page_xy_addr, &cpu.Y); }
static char *test_op_8C  (void) { return test_str(0x8C, 4, "STY, Absolute (0x8C)",       load_absolute_addr, &cpu.Y); }

static char *test_op_AA  (void) { return test_trn(0xAA,    "TAX, Implied (0xAA)",        &cpu.A, &cpu.X, true ); }
static char *test_op_A8  (void) { return test_trn(0xA8,    "TAY, Implied (0xA8)",        &cpu.A, &cpu.Y, true ); }
static char *test_op_BA  (void) { return test_trn(0xBA,    "TSX, Implied (0xBA)",        &cpu.S, &cpu.X, true ); }
static char *test_op_8A  (void) { return test_trn(0x8A,    "TXA, Implied (0x8A)",        &cpu.X, &cpu.A, true ); }
static char *test_op_9A  (void) { return test_trn(0x9A,    "TXS, Implied (0x9A)",        &cpu.X, &cpu.S, false); }
static char *test_op_98  (void) { return test_trn(0x98,    "TYA, Implied (0x98)",        &cpu.Y, &cpu.A, true ); }

char *cpu_tests(void) {
    setup();

    RUN_TEST( test_op_69 );     /* ADC, Immediate. */
    RUN_TEST( test_op_65 );     /* ADC, Zero page. */
    RUN_TEST( test_op_75 );     /* ADC, Zero page X. */
    RUN_TEST( test_op_6D );     /* ADC, Absolute. */
    RUN_TEST( test_op_7D_1 );   /* ADC, Absolute X. */
    RUN_TEST( test_op_7D_2 );   /* ADC, Absolute X (P). */
    RUN_TEST( test_op_79_1 );   /* ADC, Absolute Y. */
    RUN_TEST( test_op_79_2 );   /* ADC, Absolute Y (P). */
    RUN_TEST( test_op_61 );     /* ADC, Indirect X. */
    RUN_TEST( test_op_71_1 );   /* ADC, Indirect Y. */
    RUN_TEST( test_op_71_2 );   /* ADC, Indirect Y (P). */

    RUN_TEST( test_op_29 );     /* AND, Immediate. */
    RUN_TEST( test_op_25 );     /* AND, Zero page. */
    RUN_TEST( test_op_35 );     /* AND, Zero page X. */
    RUN_TEST( test_op_2D );     /* AND, Absolute. */
    RUN_TEST( test_op_3D_1 );   /* AND, Absolute X. */
    RUN_TEST( test_op_3D_2 );   /* AND, Absolute X (P). */
    RUN_TEST( test_op_39_1 );   /* AND, Absolute Y. */
    RUN_TEST( test_op_39_2 );   /* AND, Absolute Y (P). */
    RUN_TEST( test_op_21 );     /* AND, Indirect X. */
    RUN_TEST( test_op_31_1 );   /* AND, Indirect Y. */
    RUN_TEST( test_op_31_2 );   /* AND, Indirect Y (P). */

    RUN_TEST( test_op_0A );     /* ASL, Accumulator. */
    RUN_TEST( test_op_06 );     /* ASL, Zero page. */
    RUN_TEST( test_op_16 );     /* ASL, Zero page X. */
    RUN_TEST( test_op_0E );     /* ASL, Absolute. */
    RUN_TEST( test_op_1E );     /* ASL, Absolute X. */

    RUN_TEST( test_op_24 );     /* BIT, Zero page. */
    RUN_TEST( test_op_2C );     /* BIT, Absolute. */

    RUN_TEST( test_op_90 );     /* BCC, Relative. */
    RUN_TEST( test_op_B0 );     /* BCS, Relative. */
    RUN_TEST( test_op_F0 );     /* BEQ, Relative. */
    RUN_TEST( test_op_30 );     /* BMI, Relative. */
    RUN_TEST( test_op_D0 );     /* BNE, Relative. */
    RUN_TEST( test_op_10 );     /* BPL, Relative. */
    RUN_TEST( test_op_50 );     /* BVC, Relative. */
    RUN_TEST( test_op_70 );     /* BVS, Relative. */

    RUN_TEST( test_op_00_40 );  /* BRK and RTI. */

    RUN_TEST( test_op_18 );     /* CLC, Implied. */
    RUN_TEST( test_op_D8 );     /* CLD, Implied. */
    RUN_TEST( test_op_58 );     /* CLI, Implied. */
    RUN_TEST( test_op_B8 );     /* CLV, Implied. */

    RUN_TEST( test_op_C9 );     /* CMP, Immediate. */
    RUN_TEST( test_op_C5 );     /* CMP, Zero page. */
    RUN_TEST( test_op_D5 );     /* CMP, Zero page X. */
    RUN_TEST( test_op_CD );     /* CMP, Absolute. */
    RUN_TEST( test_op_DD_1 );   /* CMP, Absolute X. */
    RUN_TEST( test_op_DD_2 );   /* CMP, Absolute X (P). */
    RUN_TEST( test_op_D9_1 );   /* CMP, Absolute Y. */
    RUN_TEST( test_op_D9_2 );   /* CMP, Absolute Y (P). */
    RUN_TEST( test_op_C1 );     /* CMP, Indirect X. */
    RUN_TEST( test_op_D1_1 );   /* CMP, Indirect Y. */
    RUN_TEST( test_op_D1_2 );   /* CMP, Indirect Y (P). */
    RUN_TEST( test_op_E0 );     /* CPX, Immediate. */
    RUN_TEST( test_op_E4 );     /* CPX, Zero page. */
    RUN_TEST( test_op_EC );     /* CPX, Absolute. */
    RUN_TEST( test_op_C0 );     /* CPY, Immediate. */
    RUN_TEST( test_op_C4 );     /* CPY, Zero page. */
    RUN_TEST( test_op_CC );     /* CPY, Absolute. */

    RUN_TEST( test_op_C6 );     /* DEC, Zero page. */
    RUN_TEST( test_op_D6 );     /* DEC, Zero page X. */
    RUN_TEST( test_op_CE );     /* DEC, Absolute. */
    RUN_TEST( test_op_DE );     /* DEC, Absolute X. */
    RUN_TEST( test_op_CA );     /* DEX, Implied. */
    RUN_TEST( test_op_88 );     /* DEY, Implied. */

    RUN_TEST( test_op_49 );     /* EOR, Immediate. */
    RUN_TEST( test_op_45 );     /* EOR, Zero page. */
    RUN_TEST( test_op_55 );     /* EOR, Zero page X. */
    RUN_TEST( test_op_4D );     /* EOR, Absolute. */
    RUN_TEST( test_op_5D_1 );   /* EOR, Absolute X. */
    RUN_TEST( test_op_5D_2 );   /* EOR, Absolute X (P). */
    RUN_TEST( test_op_59_1 );   /* EOR, Absolute Y. */
    RUN_TEST( test_op_59_2 );   /* EOR, Absolute Y (P). */
    RUN_TEST( test_op_41 );     /* EOR, Indirect X. */
    RUN_TEST( test_op_51_1 );   /* EOR, Indirect Y. */
    RUN_TEST( test_op_51_2 );   /* EOR, Indirect Y (P). */

    RUN_TEST( test_op_E6 );     /* INC, Zero page. */
    RUN_TEST( test_op_F6 );     /* INC, Zero page X. */
    RUN_TEST( test_op_EE );     /* INC, Absolute. */
    RUN_TEST( test_op_FE );     /* INC, Absolute X. */
    RUN_TEST( test_op_E8 );     /* INX, Implied. */
    RUN_TEST( test_op_C8 );     /* INY, Implied. */

    RUN_TEST( test_op_4C_6C );  /* JMP, Absolute and Indirect. */

    RUN_TEST( test_op_20_60 );  /* JSR and RTS. */

    RUN_TEST( test_op_09 );     /* ORA, Immediate. */
    RUN_TEST( test_op_05 );     /* ORA, Zero page. */
    RUN_TEST( test_op_15 );     /* ORA, Zero page X. */
    RUN_TEST( test_op_0D );     /* ORA, Absolute. */
    RUN_TEST( test_op_1D_1 );   /* ORA, Absolute X. */
    RUN_TEST( test_op_1D_2 );   /* ORA, Absolute X (P). */
    RUN_TEST( test_op_19_1 );   /* ORA, Absolute Y. */
    RUN_TEST( test_op_19_2 );   /* ORA, Absolute Y (P). */
    RUN_TEST( test_op_01 );     /* ORA, Indirect X. */
    RUN_TEST( test_op_11_1 );   /* ORA, Indirect Y. */
    RUN_TEST( test_op_11_2 );   /* ORA, Indirect Y (P). */

    RUN_TEST( test_op_A9 );     /* LDA, Immediate. */
    RUN_TEST( test_op_A5 );     /* LDA, Zero page. */
    RUN_TEST( test_op_B5 );     /* LDA, Zero page X. */
    RUN_TEST( test_op_AD );     /* LDA, Absolute. */
    RUN_TEST( test_op_BD_1 );   /* LDA, Absolute X. */
    RUN_TEST( test_op_BD_2 );   /* LDA, Absolute X (P). */
    RUN_TEST( test_op_B9_1 );   /* LDA, Absolute Y. */
    RUN_TEST( test_op_B9_2 );   /* LDA, Absolute Y (P). */
    RUN_TEST( test_op_A1 );     /* LDA, Indirect X. */
    RUN_TEST( test_op_B1_1 );   /* LDA, Indirect Y. */
    RUN_TEST( test_op_B1_2 );   /* LDA, Indirect Y (P). */
    RUN_TEST( test_op_A2 );     /* LDX, Immediate. */
    RUN_TEST( test_op_A6 );     /* LDX, Zero page. */
    RUN_TEST( test_op_B6 );     /* LDX, Zero page Y. */
    RUN_TEST( test_op_AE );     /* LDX, Absolute. */
    RUN_TEST( test_op_BE_1 );   /* LDX, Absolute Y. */
    RUN_TEST( test_op_BE_2 );   /* LDX, Absolute Y (P). */
    RUN_TEST( test_op_A0 );     /* LDY, Immediate. */
    RUN_TEST( test_op_A4 );     /* LDY, Zero page. */
    RUN_TEST( test_op_B4 );     /* LDY, Zero page Y. */
    RUN_TEST( test_op_AC );     /* LDY, Absolute. */
    RUN_TEST( test_op_BC_1 );   /* LDY, Absolute X. */
    RUN_TEST( test_op_BC_2 );   /* LDY, Absolute X (P). */

    RUN_TEST( test_op_4A );     /* LSR, Accumulator. */
    RUN_TEST( test_op_46 );     /* LSR, Zero page. */
    RUN_TEST( test_op_56 );     /* LSR, Zero page X. */
    RUN_TEST( test_op_4E );     /* LSR, Absolute. */
    RUN_TEST( test_op_5E );     /* LSR, Absolute X. */

    RUN_TEST( test_op_48 );     /* PHA, Implied. */
    RUN_TEST( test_op_08 );     /* PHP, Implied. */
    RUN_TEST( test_op_68 );     /* PLA, Implied. */
    RUN_TEST( test_op_28 );     /* PLP, Implied. */

    RUN_TEST( test_op_2A );     /* ROL, Accumulator. */
    RUN_TEST( test_op_26 );     /* ROL, Zero page. */
    RUN_TEST( test_op_36 );     /* ROL, Zero page X. */
    RUN_TEST( test_op_2E );     /* ROL, Absolute. */
    RUN_TEST( test_op_3E );     /* ROL, Absolute X. */

    RUN_TEST( test_op_6A );     /* ROR, Accumulator. */
    RUN_TEST( test_op_66 );     /* ROR, Zero page. */
    RUN_TEST( test_op_76 );     /* ROR, Zero page X. */
    RUN_TEST( test_op_6E );     /* ROR, Absolute. */
    RUN_TEST( test_op_7E );     /* ROR, Absolute X. */

    RUN_TEST( test_op_E9 );     /* SBC, Immediate. */
    RUN_TEST( test_op_E5 );     /* SBC, Zero page. */
    RUN_TEST( test_op_F5 );     /* SBC, Zero page X. */
    RUN_TEST( test_op_ED );     /* SBC, Absolute. */
    RUN_TEST( test_op_FD_1 );   /* SBC, Absolute X. */
    RUN_TEST( test_op_FD_2 );   /* SBC, Absolute X (P). */
    RUN_TEST( test_op_F9_1 );   /* SBC, Absolute Y. */
    RUN_TEST( test_op_F9_2 );   /* SBC, Absolute Y (P). */
    RUN_TEST( test_op_E1 );     /* SBC, Indirect X. */
    RUN_TEST( test_op_F1_1 );   /* SBC, Indirect Y. */
    RUN_TEST( test_op_F1_2 );   /* SBC, Indirect Y (P). */

    RUN_TEST( test_op_38 );     /* SEC, Implied. */
    RUN_TEST( test_op_F8 );     /* SED, Implied. */
    RUN_TEST( test_op_78 );     /* SEI, Implied. */

    RUN_TEST( test_op_85 );     /* STA, Zero page. */
    RUN_TEST( test_op_95 );     /* STA, Zero page X. */
    RUN_TEST( test_op_8D );     /* STA, Absolute. */
    RUN_TEST( test_op_9D );     /* STA, Absolute X. */
    RUN_TEST( test_op_99 );     /* STA, Absolute Y. */
    RUN_TEST( test_op_81 );     /* STA, Indirect X. */
    RUN_TEST( test_op_91 );     /* STA, Indirect Y. */
    RUN_TEST( test_op_86 );     /* STX, Zero page. */
    RUN_TEST( test_op_96 );     /* STX, Zero page Y. */
    RUN_TEST( test_op_8E );     /* STX, Absolute. */
    RUN_TEST( test_op_84 );     /* STY, Zero page. */
    RUN_TEST( test_op_94 );     /* STY, Zero page X. */
    RUN_TEST( test_op_8C );     /* STY, Absolute. */

    RUN_TEST( test_op_AA );     /* TAX, Implied. */
    RUN_TEST( test_op_A8 );     /* TAY, Implied. */
    RUN_TEST( test_op_BA );     /* TSX, Implied. */
    RUN_TEST( test_op_8A );     /* TXA, Implied. */
    RUN_TEST( test_op_9A );     /* TXS, Implied. */
    RUN_TEST( test_op_98 );     /* TYA, Implied. */

    return 0;
}
