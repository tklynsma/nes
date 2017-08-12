#include "../../src/cpu.c"
#include "../../src/cpu_flags.c"
#include "../../src/memory.c"
#include "cpu_tests.h"
#include "test_unit.h"

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

static inline void load_zero_page_x_addr(word pc, byte op, word address) {
    cpu_reset();
    cpu.PC = pc;
    mem_write(pc, op);
    mem_write(pc + 1, address & 0xFF);
    cpu.X = 0x00;
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

static char *test_branching(byte opcode, char *msg, Function set, Function clear) {
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

static char *test_clear(byte opcode, char *msg, Function set, Bool check) {
    load_implied(0x8000, opcode);
    (*set)();
    cpu_cycle(2);
    ASSERT(msg, wait_cycles == 0);
    ASSERT(msg, !(*check)());

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

static char *test_store(byte opcode, int cycles, char *msg, LoadAddress load, byte *store) {
    (*load)(0x8000, opcode, 0x00AB);
    *store = 0x77;
    cpu_cycle(cycles);
    ASSERT(msg, wait_cycles == 0);
    ASSERT(msg, mem_read_byte(0x00AB) == 0x77);

    return 0;
}

static char *test_opcode_29  (void) { return test_and(0x29, 2, "AND, Immediate (0x29)", load_immediate); }
static char *test_opcode_25  (void) { return test_and(0x25, 3, "AND, Zero page (0x25)", load_zero_page); }
static char *test_opcode_35  (void) { return test_and(0x35, 4, "AND, Zero page X (0x35)", load_zero_page_x); }
static char *test_opcode_2D  (void) { return test_and(0x2D, 4, "AND, Absolute (0x2D)", load_absolute); }
static char *test_opcode_3D_1(void) { return test_and(0x3D, 4, "AND, Absolute X (0x3D)", load_absolute_x); }
static char *test_opcode_3D_2(void) { return test_and(0x3D, 5, "AND, Absolute X (0x3D) (P)", load_absolute_x_page); }
static char *test_opcode_39_1(void) { return test_and(0x39, 4, "AND, Absolute Y (0x39)", load_absolute_y); }
static char *test_opcode_39_2(void) { return test_and(0x39, 5, "AND, Absolute Y (0x39) (P)", load_absolute_y_page); }
static char *test_opcode_21  (void) { return test_and(0x21, 6, "AND, Indirect X (0x21)", load_indirect_x); }
static char *test_opcode_31_1(void) { return test_and(0x31, 5, "AND, Indirect Y (0x31)", load_indirect_y); }
static char *test_opcode_31_2(void) { return test_and(0x31, 6, "AND, Indirect Y (0x31) (P)", load_indirect_y_page); }

static char *test_opcode_B0  (void) { return test_branching(0xB0, "BCS (0xB0)", flg_set_C, flg_clear_C); }
static char *test_opcode_90  (void) { return test_branching(0x90, "BCC (0x90)", flg_clear_C, flg_set_C); }
static char *test_opcode_F0  (void) { return test_branching(0xF0, "BEQ (0xF0)", flg_set_Z, flg_clear_Z); }
static char *test_opcode_D0  (void) { return test_branching(0xD0, "BNE (0xD0)", flg_clear_Z, flg_set_Z); }
static char *test_opcode_70  (void) { return test_branching(0x70, "BVS (0x70)", flg_set_V, flg_clear_V); }
static char *test_opcode_50  (void) { return test_branching(0x50, "BVS (0x50)", flg_clear_V, flg_set_V); }
static char *test_opcode_30  (void) { return test_branching(0x30, "BMI (0x30)", flg_set_N, flg_clear_N); }
static char *test_opcode_10  (void) { return test_branching(0x10, "BPL (0x10)", flg_clear_N, flg_set_N); }

static char *test_opcode_18  (void) { return test_clear(0x18, "CLC (0x18)", flg_set_C, flg_is_C); }
static char *test_opcode_D8  (void) { return test_clear(0xD8, "CLD (0xD8)", flg_set_D, flg_is_D); }
static char *test_opcode_58  (void) { return test_clear(0x58, "CLI (0x58)", flg_set_I, flg_is_I); }
static char *test_opcode_B8  (void) { return test_clear(0xB8, "CLV (0xB8)", flg_set_V, flg_is_V); }

static char *test_opcode_49  (void) { return test_eor(0x49, 2, "EOR, Immediate (0x49)", load_immediate); }
static char *test_opcode_45  (void) { return test_eor(0x45, 3, "EOR, Zero page (0x45)", load_zero_page); }
static char *test_opcode_55  (void) { return test_eor(0x55, 4, "EOR, Zero page X (0x55)", load_zero_page_x); }
static char *test_opcode_4D  (void) { return test_eor(0x4D, 4, "EOR, Absolute (0x4D)", load_absolute); }
static char *test_opcode_5D_1(void) { return test_eor(0x5D, 4, "EOR, Absolute X (0x5D)", load_absolute_x); }
static char *test_opcode_5D_2(void) { return test_eor(0x5D, 5, "EOR, Absolute X (0x5D) (P)", load_absolute_x_page); }
static char *test_opcode_59_1(void) { return test_eor(0x59, 4, "EOR, Absolute Y (0x59)", load_absolute_y); }
static char *test_opcode_59_2(void) { return test_eor(0x59, 5, "EOR, Absolute Y (0x59) (P)", load_absolute_y_page); }
static char *test_opcode_41  (void) { return test_eor(0x41, 6, "EOR, Indirect X (0x41)", load_indirect_x); }
static char *test_opcode_51_1(void) { return test_eor(0x51, 5, "EOR, Indirect Y (0x51)", load_indirect_y); }
static char *test_opcode_51_2(void) { return test_eor(0x51, 6, "EOR, Indirect Y (0x51) (P)", load_indirect_y_page); }

static char *test_opcode_09  (void) { return test_ora(0x09, 2, "ORA, Immediate (0x09)", load_immediate); }
static char *test_opcode_05  (void) { return test_ora(0x05, 3, "ORA, Zero page (0x05)", load_zero_page); }
static char *test_opcode_15  (void) { return test_ora(0x15, 4, "ORA, Zero page X (0x15)", load_zero_page_x); }
static char *test_opcode_0D  (void) { return test_ora(0x0D, 4, "ORA, Absolute (0x0D)", load_absolute); }
static char *test_opcode_1D_1(void) { return test_ora(0x1D, 4, "ORA, Absolute X (0x1D)", load_absolute_x); }
static char *test_opcode_1D_2(void) { return test_ora(0x1D, 5, "ORA, Absolute X (0x1D) (P)", load_absolute_x_page); }
static char *test_opcode_19_1(void) { return test_ora(0x19, 4, "ORA, Absolute Y (0x19)", load_absolute_y); }
static char *test_opcode_19_2(void) { return test_ora(0x19, 5, "ORA, Absolute Y (0x19) (P)", load_absolute_y_page); }
static char *test_opcode_01  (void) { return test_ora(0x01, 6, "ORA, Indirect X (0x01)", load_indirect_x); }
static char *test_opcode_11_1(void) { return test_ora(0x11, 5, "ORA, Indirect Y (0x11)", load_indirect_y); }
static char *test_opcode_11_2(void) { return test_ora(0x11, 6, "ORA, Indirect Y (0x11) (P)", load_indirect_y_page); }

static char *test_opcode_38  (void) { return test_set(0x38, "SEC (0x38)", flg_clear_C, flg_is_C); }
static char *test_opcode_F8  (void) { return test_set(0xF8, "SED (0xF8)", flg_clear_D, flg_is_D); }
static char *test_opcode_78  (void) { return test_set(0x78, "SEI (0x78)", flg_clear_I, flg_is_I); }

static char *test_opcode_85  (void) { return test_store(0x85, 3, "STA, Zero page (0x85)", load_zero_page_addr, &cpu.A); }
static char *test_opcode_95  (void) { return test_store(0x95, 4, "STA, Zero page X (0x95)", load_zero_page_x_addr, &cpu.A); }
static char *test_opcode_8D  (void) { return test_store(0x8D, 4, "STA, Absolute (0x8D)", load_absolute_addr, &cpu.A); }
static char *test_opcode_9D  (void) { return test_store(0x9D, 5, "STA, Absolute X (0x9D)", load_absolute_x_addr, &cpu.A); }
static char *test_opcode_99  (void) { return test_store(0x99, 5, "STA, Absolute Y (0x99)", load_absolute_y_addr, &cpu.A); }

/* JMP, Absolute. */
static char *test_opcode_4C(void) {
    cpu_reset();
    cpu.PC = 0x8000;
    mem_write(0x8000, 0x4C);
    mem_write(0x8001, 0x34);
    mem_write(0x8002, 0x12);
    cpu_cycle(3);
    ASSERT("JMP, Absolute (0x4C)", cpu.PC == 0x1234);
    ASSERT("JMP, Absolute (0x4C)", wait_cycles == 0);
}

/* JMP, Indirect. */
static char *test_opcode_6C(void) {
    cpu_reset();
    cpu.PC = 0x8000;
    mem_write(0x8000, 0x6C);
    mem_write(0x8001, 0x34);
    mem_write(0x8002, 0x02);
    mem_write(0x0234, 0xCD);
    mem_write(0x0235, 0xAB);
    cpu_cycle(5);
    ASSERT("JMP, Indirect (0x6C)", cpu.PC == 0xABCD);
    ASSERT("JMP, Indirect (0x6C)", wait_cycles == 0);
}

/* NOP, Implied. */
static char *test_opcode_EA(void) {
    load_implied(0x8000, 0xEA);
    cpu_cycle(2);
    ASSERT("NOP (0xEA)", wait_cycles == 0);
}

char *cpu_tests(void) {
    cpu_init();

    /* AND - Logical AND. */
    RUN_TEST( test_opcode_29 );
    RUN_TEST( test_opcode_25 );
    RUN_TEST( test_opcode_35 );
    RUN_TEST( test_opcode_2D );
    RUN_TEST( test_opcode_3D_1 );
    RUN_TEST( test_opcode_3D_2 );
    RUN_TEST( test_opcode_39_1 );
    RUN_TEST( test_opcode_39_2 );
    RUN_TEST( test_opcode_21 );
    RUN_TEST( test_opcode_31_1 );
    RUN_TEST( test_opcode_31_2 );

    /* Branch instructions. */
    RUN_TEST( test_opcode_B0 );
    RUN_TEST( test_opcode_90 );
    RUN_TEST( test_opcode_F0 );
    RUN_TEST( test_opcode_D0 );
    RUN_TEST( test_opcode_70 );
    RUN_TEST( test_opcode_50 );
    RUN_TEST( test_opcode_30 );
    RUN_TEST( test_opcode_10 );

    /* Clear instructions. */
    RUN_TEST( test_opcode_18 );
    RUN_TEST( test_opcode_D8 );
    RUN_TEST( test_opcode_58 );
    RUN_TEST( test_opcode_B8 );

    /* EOR - Exclusive OR. */
    RUN_TEST( test_opcode_49 );
    RUN_TEST( test_opcode_45 );
    RUN_TEST( test_opcode_55 );
    RUN_TEST( test_opcode_4D );
    RUN_TEST( test_opcode_5D_1 );
    RUN_TEST( test_opcode_5D_2 );
    RUN_TEST( test_opcode_59_1 );
    RUN_TEST( test_opcode_59_2 );
    RUN_TEST( test_opcode_41 );
    RUN_TEST( test_opcode_51_1 );
    RUN_TEST( test_opcode_51_2 );

    /* JMP - Jump. */
    RUN_TEST( test_opcode_4C );
    RUN_TEST( test_opcode_6C );

    /* NOP - No operation. */
    RUN_TEST( test_opcode_EA );

    /* ORA - Logical Inclusive OR. */
    RUN_TEST( test_opcode_09 );
    RUN_TEST( test_opcode_05 );
    RUN_TEST( test_opcode_15 );
    RUN_TEST( test_opcode_0D );
    RUN_TEST( test_opcode_1D_1 );
    RUN_TEST( test_opcode_1D_2 );
    RUN_TEST( test_opcode_19_1 );
    RUN_TEST( test_opcode_19_2 );
    RUN_TEST( test_opcode_01 );
    RUN_TEST( test_opcode_11_1 );
    RUN_TEST( test_opcode_11_2 );

    /* Set instructions. */
    RUN_TEST( test_opcode_38 );
    RUN_TEST( test_opcode_F8 );
    RUN_TEST( test_opcode_78 );

    /* Store instructions. */
    RUN_TEST( test_opcode_85 );
    RUN_TEST( test_opcode_95 );
    RUN_TEST( test_opcode_8D );
    RUN_TEST( test_opcode_9D );
    RUN_TEST( test_opcode_99 );

    return 0;
}
