#ifndef TEST_UNIT_H
#define TEST_UNIT_H

typedef char *(*Test)(void);

/* Based on MinUnit. */
#define ASSERT(message, test) do { assertions_run++; if (!(test)) return message; } while (0)
#define RUN_TEST(test) do { char *message = test(); tests_run++; if (message) return message; else printf("."); } while (0)

int assertions_run;
int tests_run;

void run_tests(char* name, Test test);

#endif /* TEST_UNIT_H */
