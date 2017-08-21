#include <stdio.h>
#include "../include/cpu_tests.h"
#include "../include/memory_tests.h"
#include "../include/test_unit.h"
#include "../include/vram_tests.h"

int main(int argc, char **argv) {
    run_tests("memory", mem_tests);
    run_tests("cpu",    cpu_tests);
    run_tests("vram",   vrm_tests);
    return 0;
}
