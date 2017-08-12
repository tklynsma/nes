#include <stdio.h>
#include "cpu_tests.h"
#include "test_unit.h"

int main(int argc, char **argv) {
    run_tests("cpu", cpu_tests);
    return 0;
}
