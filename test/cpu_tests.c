#include "../src/cpu.c"
#include "../src/cpu_flags.c"
#include "../src/memory.c"
#include "cpu_tests.h"
#include "testunit.h"

/* JMP, Absolute. */
static char *test_opcode_4C(void) {
    cpu_reset();
    cpu.pc = 0x8000;
    mem_write(0x8000, 0x4C);
    mem_write(0x8001, 0x34);
    mem_write(0x8002, 0x12);
    cpu_cycle(3);
    ASSERT("op_4C_PC", cpu.pc == 0x1234);
    ASSERT("op_4C_cycles", wait_cycles == 0);
    return 0;
}

/* JMP, Indirect. */
static char *test_opcode_6C(void) {
    cpu_reset();
    cpu.pc = 0x8000;
    mem_write(0x8000, 0x6C);
    mem_write(0x8001, 0x34);
    mem_write(0x8002, 0x02);
    mem_write(0x0234, 0xCD);
    mem_write(0x0235, 0xAB);
    cpu_cycle(5);
    ASSERT("op_6C_PC", cpu.pc == 0xABCD);
    ASSERT("op_6C_cycles", wait_cycles == 0);
    return 0;
}

/* NOP, Implied. */
static char *test_opcode_EA(void) {
    cpu_reset();
    cpu.pc = 0x8000;
    mem_write(0x8000, 0xEA);
    cpu_cycle(2);
    ASSERT("op_EA_PC", cpu.pc == 0x8001);
    ASSERT("op_EA_cycles", wait_cycles == 0);
    return 0;
}

char *cpu_tests(void) {
    cpu_init();

    RUN_TEST(test_opcode_4C);
    RUN_TEST(test_opcode_6C);
    RUN_TEST(test_opcode_EA);

    return 0;
}
