#include "../../src/cpu.c"
#include "../../src/cpu_flags.c"
#include "../../src/memory.c"
#include "../include/cpu_tests.h"
#include "../include/test_unit.h"

/* -----------------------------------------------------------------
 * Help functions for loading instructions.
 * -------------------------------------------------------------- */

typedef bool (*Bool)(void);
typedef void (*LoadOperand)(word, byte, byte, byte);
typedef void (*LoadAddress)(word, byte, word);

static inline void load_absolute(word pc, byte op, byte arg, byte acc) {
    cpu_reset();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, 0x34);
    mem_write(pc + 2, 0x12);
    mem_write(0x1234, arg);
    cpu.A = acc;
}

static inline void load_absolute_addr(word pc, byte op, word address) {
    cpu_reset();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, address & 0xFF);
    mem_write(pc + 2, address >> 8);
}

static inline void load_absolute_x(word pc, byte op, byte arg, byte acc) {
    cpu_reset();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, 0x00);
    mem_write(pc + 2, 0x12);
    mem_write(0x1234, arg);
    cpu.A = acc;
    cpu.X = 0x34;
}

static inline void load_absolute_x_addr(word pc, byte op, word address) {
    cpu_reset();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, address & 0xFF);
    mem_write(pc + 2, address >> 8);
    cpu.X = 0x00;
}

static inline void load_absolute_x_page(word pc, byte op, byte arg, byte acc) {
    cpu_reset();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, 0xFF);
    mem_write(pc + 2, 0x12);
    mem_write(0x1300, arg);
    cpu.A = acc;
    cpu.X = 0x01;
}

static inline void load_absolute_y(word pc, byte op, byte arg, byte acc) {
    cpu_reset();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, 0x00);
    mem_write(pc + 2, 0x12);
    mem_write(0x1234, arg);
    cpu.A = acc;
    cpu.Y = 0x34;
}

static inline void load_absolute_y_addr(word pc, byte op, word address) {
    cpu_reset();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, address & 0xFF);
    mem_write(pc + 2, address >> 8);
    cpu.Y = 0x00;
}

static inline void load_absolute_y_page(word pc, byte op, byte arg, byte acc) {
    cpu_reset();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, 0xFF);
    mem_write(pc + 2, 0x12);
    mem_write(0x1300, arg);
    cpu.A = acc;
    cpu.Y = 0x01;
}

static inline void load_immediate(word pc, byte op, byte arg, byte acc) {
    cpu_reset();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, arg);
    cpu.A = acc;
}

static inline void load_implied(word pc, byte op) {
    cpu_reset();
    cpu.PC = pc;
    mem_write(pc, op);
}

static inline void load_indirect_addr(word pc, byte op, word address) {
    cpu_reset();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, 0x34);
    mem_write(pc + 2, 0x12);
    mem_write(0x1234, address & 0xFF);
    mem_write(0x1235, address >> 8);
}

static inline void load_indirect_x(word pc, byte op, byte arg, byte acc) {
    cpu_reset();
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
    cpu_reset();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, 0xA0);
    mem_write(0x00AB, address & 0xFF);
    mem_write(0x00AC, address >> 8);
    cpu.X = 0x0B;
}

static inline void load_indirect_y(word pc, byte op, byte arg, byte acc) {
    cpu_reset();
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
    cpu_reset();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, 0xAB);
    mem_write(0x00AB, address & 0xFF);
    mem_write(0x00AC, address >> 8);
    cpu.Y = 0x00;
}

static inline void load_indirect_y_page(word pc, byte op, byte arg, byte acc) {
    cpu_reset();
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
    cpu_reset();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, 0xAB);
    mem_write(0x00AB, arg);
    cpu.A = acc;
}

static inline void load_zero_page_addr(word pc, byte op, word address) {
    cpu_reset();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, address & 0xFF);
}

static inline void load_zero_page_x(word pc, byte op, byte arg, byte acc) {
    cpu_reset();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, 0xA0);
    mem_write(0x00AB, arg);
    cpu.A = acc;
    cpu.X = 0x0B;
}

static inline void load_zero_page_xy_addr(word pc, byte op, word address) {
    cpu_reset();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, address & 0xFF);
    cpu.X = 0x00;
    cpu.Y = 0x00;
}

/* -----------------------------------------------------------------
 * CPU instruction tests.
 * -------------------------------------------------------------- */

static char *test_and(byte opcode, int cycles, char *msg, LoadOperand load) {
    /* No flags set. */
    (*load)(0x8000, opcode, 0x77, 0xBB);
    cpu_cycle(cycles);
    ASSERT(msg, wait_cycles == 0);
    ASSERT(msg, cpu.A == 0x33);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Zero flag set. */
    (*load)(0x8000, opcode, 0x88, 0x33);
    cpu_cycle(cycles);
    ASSERT(msg, cpu.A == 0x00);
    ASSERT(msg, flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Negative flag set. */
    (*load)(0x8000, opcode, 0xBB, 0xFF);
    cpu_cycle(cycles);
    ASSERT(msg, cpu.A == 0xBB);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, flg_is_N());

    return 0;
}

static char *test_brc(byte opcode, char *msg, Function set, Function clear) {
    /* Branching fails. */
    load_relative(0x8000, opcode, 0x75);
    (*clear)();
    cpu_cycle(2);
    ASSERT(msg, wait_cycles == 0);
    ASSERT(msg, cpu.PC == 0x8002);

    /* Branching succeeds, positive offset, no page crossed. */
    load_relative(0x80FE, opcode, 0x77);
    (*set)();
    cpu_cycle(3);
    ASSERT(msg, wait_cycles == 0);
    ASSERT(msg, cpu.PC == 0x8177);

    /* Branching succeeds, negative offset, no page crossed. */
    load_relative(0x8075, opcode, 0x89);
    (*set)();
    cpu_cycle(3);
    ASSERT(msg, wait_cycles == 0);
    ASSERT(msg, cpu.PC == 0x8000);

    /* Branching succeeds, positive offset, page crossed. */
    load_relative(0x80FD, opcode, 0x01);
    (*set)();
    cpu_cycle(4);
    ASSERT(msg, wait_cycles == 0);
    ASSERT(msg, cpu.PC == 0x8100);

    /* Branching succeeds, negative offset, page crossed. */
    load_relative(0x8000, opcode, 0xFD);
    (*set)();
    cpu_cycle(4);
    ASSERT(msg, wait_cycles == 0);
    ASSERT(msg, cpu.PC == 0x7FFF);

    return 0;
}

static char *test_clr(byte opcode, char *msg, Function set, Bool check) {
    load_implied(0x8000, opcode);
    (*set)();
    cpu_cycle(2);
    ASSERT(msg, wait_cycles == 0);
    ASSERT(msg, !(*check)());

    return 0;
}

static char *test_dec(byte opcode, int cycles, char *msg, LoadAddress load) {
    (*load)(0x8000, opcode, 0x00AB);
    mem_write(0x00AB, 0x77);
    cpu_cycle(cycles);
    ASSERT(msg, wait_cycles == 0);
    ASSERT(msg, mem_read_byte(0x00AB) == 0x76);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Flag Z is set. */
    (*load)(0x8000, opcode, 0x00AB);
    mem_write(0x00AB, 0x01);
    cpu_cycle(cycles);
    ASSERT(msg, mem_read_byte(0x00AB) == 0x00);
    ASSERT(msg, flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Flag N is set. */
    (*load)(0x8000, opcode, 0x00AB);
    mem_write(0x00AB, 0x00);
    cpu_cycle(cycles);
    ASSERT(msg, mem_read_byte(0x00AB) == 0xFF);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, flg_is_N());

    return 0;
}

static char *test_dex(byte opcode, char *msg, byte *reg) {
    load_implied(0x8000, opcode);
    *reg = 0x77;
    cpu_cycle(2);
    ASSERT(msg, wait_cycles == 0);
    ASSERT(msg, *reg == 0x76);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Flag Z is set. */
    load_implied(0x8000, opcode);
    *reg = 0x01;
    cpu_cycle(2);
    ASSERT(msg, *reg == 0x00);
    ASSERT(msg, flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Flag N is set. */
    load_implied(0x8000, opcode);
    *reg = 0x00;
    cpu_cycle(2);
    ASSERT(msg, *reg == 0xFF);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, flg_is_N());

    return 0;
}

static char *test_eor(byte opcode, int cycles, char *msg, LoadOperand load) {
    /* No flags set. */
    (*load)(0x8000, opcode, 0x03, 0x09);
    cpu_cycle(cycles);
    ASSERT(msg, wait_cycles == 0);
    ASSERT(msg, cpu.A == 0x0A);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Flag Z is set. */
    (*load)(0x8000, opcode, 0x77, 0x77);
    cpu_cycle(cycles);
    ASSERT(msg, cpu.A == 0x00);
    ASSERT(msg, flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Flag N is set. */
    (*load)(0x8000, opcode, 0x80, 0x7F);
    cpu_cycle(cycles);
    ASSERT(msg, cpu.A == 0xFF);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, flg_is_N());

    return 0;
}

static char *test_inc(byte opcode, int cycles, char *msg, LoadAddress load) {
    (*load)(0x8000, opcode, 0x00AB);
    mem_write(0x00AB, 0x77);
    cpu_cycle(cycles);
    ASSERT(msg, wait_cycles == 0);
    ASSERT(msg, mem_read_byte(0x00AB) == 0x78);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Flag Z is set. */
    (*load)(0x8000, opcode, 0x00AB);
    mem_write(0x00AB, 0xFF);
    cpu_cycle(cycles);
    ASSERT(msg, mem_read_byte(0x00AB) == 0x00);
    ASSERT(msg, flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Flag N is set. */
    (*load)(0x8000, opcode, 0x00AB);
    mem_write(0x00AB, 0xFE);
    cpu_cycle(cycles);
    ASSERT(msg, mem_read_byte(0x00AB) == 0xFF);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, flg_is_N());

    return 0;
}

static char *test_inx(byte opcode, char *msg, byte *reg) {
    load_implied(0x8000, opcode);
    *reg = 0x77;
    cpu_cycle(2);
    ASSERT(msg, wait_cycles == 0);
    ASSERT(msg, *reg == 0x78);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Flag Z is set. */
    load_implied(0x8000, opcode);
    *reg = 0xFF;
    cpu_cycle(2);
    ASSERT(msg, *reg == 0x00);
    ASSERT(msg, flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Flag N is set. */
    load_implied(0x8000, opcode);
    *reg = 0xFE;
    cpu_cycle(2);
    ASSERT(msg, *reg == 0xFF);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, flg_is_N());

    return 0;
}

static char *test_jmp(byte opcode, int cycles, char *msg, LoadAddress load) {
    (*load)(0x8000, opcode, 0x1234);
    cpu_cycle(cycles);
    ASSERT(msg, wait_cycles == 0);
    ASSERT(msg, cpu.PC == 0x1234);

    return 0;
}

static char *test_ora(byte opcode, int cycles, char *msg, LoadOperand load) {
    /* No flags set. */
    (*load)(0x8000, opcode, 0x03, 0x08);
    cpu_cycle(cycles);
    ASSERT(msg, wait_cycles == 0);
    ASSERT(msg, cpu.A == 0x0B);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Flag Z set. */
    (*load)(0x8000, opcode, 0x00, 0x00);
    cpu_cycle(cycles);
    ASSERT(msg, cpu.A == 0x00);
    ASSERT(msg, flg_is_Z());
    ASSERT(msg, !flg_is_N());

    /* Flag N set. */
    (*load)(0x8000, opcode, 0x80, 0x03);
    cpu_cycle(cycles);
    ASSERT(msg, cpu.A == 0x83);
    ASSERT(msg, !flg_is_Z());
    ASSERT(msg, flg_is_N());

    return 0;
}

static char *test_set(byte opcode, char *msg, Function clear, Bool check) {
    load_implied(0x8000, opcode);
    (*clear)();
    cpu_cycle(2);
    ASSERT(msg, wait_cycles == 0);
    ASSERT(msg, (*check)());

    return 0;
}

static char *test_str(byte opcode, int cycles, char *msg, LoadAddress load, byte *store) {
    (*load)(0x8000, opcode, 0x0012);
    *store = 0x77;
    cpu_cycle(cycles);
    ASSERT(msg, wait_cycles == 0);
    ASSERT(msg, mem_read_byte(0x0012) == 0x77);

    return 0;
}

static char *test_trn(byte opcode, char *msg, byte *source, byte *dest, bool test_flags) {
    load_implied(0x8000, opcode);
    *source = 0x77;
    *dest = 0x00;
    cpu_cycle(2);
    ASSERT(msg, wait_cycles == 0);
    ASSERT(msg, *dest == 0x77);

    if (test_flags) {
        ASSERT(msg, !flg_is_Z());
        ASSERT(msg, !flg_is_N());

        load_implied(0x8000, opcode);
        *source = 0x00;
        cpu_cycle(2);
        ASSERT(msg, *dest == 0x00);
        ASSERT(msg, flg_is_Z());
        ASSERT(msg, !flg_is_N());

        load_implied(0x8000, opcode);
        *source = 0x83;
        cpu_cycle(2);
        ASSERT(msg, *dest == 0x83);
        ASSERT(msg, !flg_is_Z());
        ASSERT(msg, flg_is_N());
    } else {
        load_implied(0x8000, opcode);
        flg_clear_Z();
        *source = 0x00;
        cpu_cycle(2);
        ASSERT(msg, *dest == 0x00);
        ASSERT(msg, !flg_is_Z());
    }

    return 0;
}

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

static char *test_op_4C  (void) { return test_jmp(0x4C, 3, "JMP, Absolute (0x4C)",       load_absolute_addr); }
static char *test_op_6C  (void) { return test_jmp(0x6C, 5, "JMP, Indirect (0x6C)",       load_indirect_addr); }

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
    cpu_init();

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

    RUN_TEST( test_op_90 );     /* BCC, Relative. */
    RUN_TEST( test_op_B0 );     /* BCS, Relative. */
    RUN_TEST( test_op_F0 );     /* BEQ, Relative. */
    RUN_TEST( test_op_30 );     /* BMI, Relative. */
    RUN_TEST( test_op_D0 );     /* BNE, Relative. */
    RUN_TEST( test_op_10 );     /* BPL, Relative. */
    RUN_TEST( test_op_50 );     /* BVC, Relative. */
    RUN_TEST( test_op_70 );     /* BVS, Relative. */

    RUN_TEST( test_op_18 );     /* CLC, Implied. */
    RUN_TEST( test_op_D8 );     /* CLD, Implied. */
    RUN_TEST( test_op_58 );     /* CLI, Implied. */
    RUN_TEST( test_op_B8 );     /* CLV, Implied. */

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

    RUN_TEST( test_op_4C );     /* JMP, Absolute. */
    RUN_TEST( test_op_6C );     /* JMP, Indirect. */

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

    RUN_TEST( test_op_38 );     /* SEC, Implied. */
    RUN_TEST( test_op_F8 );     /* SED, Implied. */
    RUN_TEST( test_op_78 );     /* SEI, Implied. */

    /* Store instructions. */
    RUN_TEST( test_op_85 );
    RUN_TEST( test_op_95 );
    RUN_TEST( test_op_8D );
    RUN_TEST( test_op_9D );
    RUN_TEST( test_op_99 );
    RUN_TEST( test_op_81 );
    RUN_TEST( test_op_91 );
    RUN_TEST( test_op_86 );
    RUN_TEST( test_op_96 );
    RUN_TEST( test_op_8E );
    RUN_TEST( test_op_84 );
    RUN_TEST( test_op_94 );
    RUN_TEST( test_op_8C );

    /* Transfer instructions. */
    RUN_TEST( test_op_AA );
    RUN_TEST( test_op_A8 );
    RUN_TEST( test_op_BA );
    RUN_TEST( test_op_8A );
    RUN_TEST( test_op_9A );
    RUN_TEST( test_op_98 );

    return 0;
}
