#include <stdio.h>
#include "../include/cpu_tests.h"
#include "../include/test_unit.h"

int main(int argc, char **argv) {
    run_tests("cpu", cpu_tests);
    return 0;
}
