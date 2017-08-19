#include "../../include/memory.h"
#include "../include/memory_tests.h"
#include "../include/test_unit.h"

#include <stdio.h>

char *test_mirror_0000_2000(void) {
    mem_init();

    mem_write(0x0000, 0x77);
    ASSERT("Mirror 0x0000-0x2000", mem_read_byte(0x0000) == 0x77);
    ASSERT("Mirror 0x0000-0x2000", mem_read_byte(0x0800) == 0x77);
    ASSERT("Mirror 0x0000-0x2000", mem_read_byte(0x1000) == 0x77);
    ASSERT("Mirror 0x0000-0x2000", mem_read_byte(0x1800) == 0x77);

    mem_write(0x17FF, 0x66);
    ASSERT("Mirror 0x0000-0x2000", mem_read_byte(0x07FF) == 0x66);
    ASSERT("Mirror 0x0000-0x2000", mem_read_byte(0x0FFF) == 0x66);
    ASSERT("Mirror 0x0000-0x2000", mem_read_byte(0x17FF) == 0x66);
    ASSERT("Mirror 0x0000-0x2000", mem_read_byte(0x1FFF) == 0x66);

    return 0;
}

char *test_mirror_2000_4000(void) {
    mem_init();

    mem_write(0x2000, 0x55);
    for (int i = 0; i < 1024; i++) {
        ASSERT("Mirror 0x2000-0x4000", mem_read_byte(0x2000 + i * 0x08) == 0x55);
    }

    mem_write(0x3004, 0x44);
    for (int i = 0; i < 1024; i++) {
        ASSERT("Mirror 0x2000-0x4000", mem_read_byte(0x2004 + i * 0x08) == 0x44);
    }

    return 0;
}

char *mem_tests(void) {
    RUN_TEST( test_mirror_0000_2000 );
    RUN_TEST( test_mirror_2000_4000 );

    return 0;
}
