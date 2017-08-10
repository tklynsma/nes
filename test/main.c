#include <stdio.h>
#include "cpu_tests.h"
#include "testunit.h"

int main(int argc, char **argv) {
    run_tests("cpu", cpu_tests);
    return 0;
}
