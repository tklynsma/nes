#include <stdio.h>
#include "testunit.h"

#define RED   "\x1B[31m"
#define GREEN "\x1B[32m"
#define RESET "\x1B[0m"

void run_tests(char* name, Test test) {
    tests_run = assertions_run = 0;
    char *result = (*test)();

    if (result != 0) {
        printf(RED "\nERROR in %s tests: " RESET "%s (assertion %d).\n", name, result, assertions_run);
    }
    else {
        printf(GREEN "\nAll %s tests passed " RESET \
               "(%d assertions in %d test cases)\n", \
               name, assertions_run_total, tests_run);
    }
}
