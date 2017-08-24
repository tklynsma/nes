#include "../../include/memory.h"
#include "../include/memory_tests.h"
#include "../include/test_unit.h"

char *test_mirror_0000_2000(void) {
    mem_init();

    mem_write(0x0000, 0x77);
    ASSERT("Mirror 0x0000-0x2000", mem_read_8(0x0000) == 0x77);
    ASSERT("Mirror 0x0000-0x2000", mem_read_8(0x0800) == 0x77);
    ASSERT("Mirror 0x0000-0x2000", mem_read_8(0x1000) == 0x77);
    ASSERT("Mirror 0x0000-0x2000", mem_read_8(0x1800) == 0x77);

    mem_write(0x17FF, 0x66);
    ASSERT("Mirror 0x0000-0x2000", mem_read_8(0x07FF) == 0x66);
    ASSERT("Mirror 0x0000-0x2000", mem_read_8(0x0FFF) == 0x66);
    ASSERT("Mirror 0x0000-0x2000", mem_read_8(0x17FF) == 0x66);
    ASSERT("Mirror 0x0000-0x2000", mem_read_8(0x1FFF) == 0x66);

    return 0;
}

char *mem_tests(void) {
    RUN_TEST( test_mirror_0000_2000 );

    return 0;
}
