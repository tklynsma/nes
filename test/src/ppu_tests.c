#include <stdio.h>

#include "../../include/memory.h"
#include "../../include/ppu.h"
#include "../../include/ppu_internal.h"
#include "../../include/vram.h"
#include "../include/ppu_tests.h"
#include "../include/test_unit.h"

char *test_write_2000(void) {
    ppu_init();

    mem_write(0x2000, 0x80);
    ASSERT("PPUCTRL (0x2000) (write)",  ppu.ctrl_nmi);
    ASSERT("PPUCTRL (0x2000) (write)", !ppu.ctrl_master_slave);
    ASSERT("PPUCTRL (0x2000) (write)", !ppu.ctrl_sprite_size);
    ASSERT("PPUCTRL (0x2000) (write)",  ppu.ctrl_background_addr == 0x0000);
    ASSERT("PPUCTRL (0x2000) (write)",  ppu.ctrl_sprite_addr == 0x0000);
    ASSERT("PPUCTRL (0x2000) (write)",  ppu.ctrl_increment == 1);
    ASSERT("PPUCTRL (0x2000) (write)", (ppu.t & 0x0C00) >> 10 == 0);
    ASSERT("PPUCTRL (0x2000) (write)",  ppu.latch == 0x80);

    mem_write(0x2000, 0x51);
    ASSERT("PPUCTRL (0x2000) (write)", !ppu.ctrl_nmi);
    ASSERT("PPUCTRL (0x2000) (write)",  ppu.ctrl_master_slave);
    ASSERT("PPUCTRL (0x2000) (write)", !ppu.ctrl_sprite_size);
    ASSERT("PPUCTRL (0x2000) (write)",  ppu.ctrl_background_addr == 0x1000);
    ASSERT("PPUCTRL (0x2000) (write)",  ppu.ctrl_sprite_addr == 0x0000);
    ASSERT("PPUCTRL (0x2000) (write)",  ppu.ctrl_increment == 1);
    ASSERT("PPUCTRL (0x2000) (write)", (ppu.t & 0x0C00) >> 10 == 1);
    ASSERT("PPUCTRL (0x2000) (write)",  ppu.latch == 0x51);

    mem_write(0x2000, 0x2A);
    ASSERT("PPUCTRL (0x2000) (write)", !ppu.ctrl_nmi);
    ASSERT("PPUCTRL (0x2000) (write)", !ppu.ctrl_master_slave);
    ASSERT("PPUCTRL (0x2000) (write)",  ppu.ctrl_sprite_size);
    ASSERT("PPUCTRL (0x2000) (write)",  ppu.ctrl_background_addr == 0x0000);
    ASSERT("PPUCTRL (0x2000) (write)",  ppu.ctrl_sprite_addr == 0x1000);
    ASSERT("PPUCTRL (0x2000) (write)",  ppu.ctrl_increment == 1);
    ASSERT("PPUCTRL (0x2000) (write)", (ppu.t & 0x0C00) >> 10 == 2);
    ASSERT("PPUCTRL (0x2000) (write)",  ppu.latch == 0x2A);

    mem_write(0x2000, 0x07);
    ASSERT("PPUCTRL (0x2000) (write)", !ppu.ctrl_nmi);
    ASSERT("PPUCTRL (0x2000) (write)", !ppu.ctrl_master_slave);
    ASSERT("PPUCTRL (0x2000) (write)", !ppu.ctrl_sprite_size);
    ASSERT("PPUCTRL (0x2000) (write)",  ppu.ctrl_background_addr == 0x0000);
    ASSERT("PPUCTRL (0x2000) (write)",  ppu.ctrl_sprite_addr == 0x0000);
    ASSERT("PPUCTRL (0x2000) (write)",  ppu.ctrl_increment == 32);
    ASSERT("PPUCTRL (0x2000) (write)", (ppu.t & 0x0C00) >> 10 == 3);
    ASSERT("PPUCTRL (0x2000) (write)",  ppu.latch == 0x07);

    return 0;
}

char *test_write_2001(void) {
    ppu_init();

    mem_write(0x2001, 0xA5);
    ASSERT("PPUMASK (0x2001) (write)",  ppu.mask_red);
    ASSERT("PPUMASK (0x2001) (write)", !ppu.mask_green);
    ASSERT("PPUMASK (0x2001) (write)",  ppu.mask_blue);
    ASSERT("PPUMASK (0x2001) (write)", !ppu.mask_sprites);
    ASSERT("PPUMASK (0x2001) (write)", !ppu.mask_background);
    ASSERT("PPUMASK (0x2001) (write)",  ppu.mask_sprites_L);
    ASSERT("PPUMASK (0x2001) (write)", !ppu.mask_background_L);
    ASSERT("PPUMASK (0x2001) (write)",  ppu.mask_grayscale);
    ASSERT("PPUMASK (0x2001) (write)",  ppu.latch == 0xA5);

    mem_write(0x2001, 0x5A);
    ASSERT("PPUMASK (0x2001) (write)", !ppu.mask_red);
    ASSERT("PPUMASK (0x2001) (write)",  ppu.mask_green);
    ASSERT("PPUMASK (0x2001) (write)", !ppu.mask_blue);
    ASSERT("PPUMASK (0x2001) (write)",  ppu.mask_sprites);
    ASSERT("PPUMASK (0x2001) (write)",  ppu.mask_background);
    ASSERT("PPUMASK (0x2001) (write)", !ppu.mask_sprites_L);
    ASSERT("PPUMASK (0x2001) (write)",  ppu.mask_background_L);
    ASSERT("PPUMASK (0x2001) (write)", !ppu.mask_grayscale);
    ASSERT("PPUMASK (0x2001) (write)",  ppu.latch == 0x5A);

    return 0;
}

char *test_read_2002(void) {
    ppu_init();

    ppu.latch = 0x55;
    ppu.status_vblank   = false;
    ppu.status_zero_hit = false;
    ppu.status_overflow = false;
    ASSERT("PPUSTATUS (0x2002) (read)", mem_read(0x2002) == 0x15);

    ppu.latch = 0x00;
    ppu.status_vblank   = true;
    ppu.status_zero_hit = false;
    ppu.status_overflow = false;
    ASSERT("PPUSTATUS (0x2002) (read)", mem_read(0x2002) == 0x80);

    ppu.latch = 0x11;
    ppu.status_vblank   = false;
    ppu.status_zero_hit = true;
    ppu.status_overflow = false;
    ASSERT("PPUSTATUS (0x2002) (read)", mem_read(0x2002) == 0x51);

    ppu.latch = 0x00;
    ppu.status_vblank   = false;
    ppu.status_zero_hit = false;
    ppu.status_overflow = true;
    ASSERT("PPUSTATUS (0x2002) (read)", mem_read(0x2002) == 0x20);

    return 0;
}

char *test_write_2003(void) {
    ppu_init();

    mem_write(0x2003, 0x33);
    ASSERT("OAMADDR (0x2003) (write)", ppu.oam_addr == 0x33);
    ASSERT("OAMADDR (0x2003) (write)", ppu.latch == 0x33);

    return 0;
}

char *test_read_2004(void) {
    ppu_init();

    mem_write(0x2003, 0x33);
    mem_write(0x2004, 0x89);
    ASSERT("OAMDATA (0x2004) (read)", ppu.oam[0x33] == 0x89);
    ASSERT("OAMDATA (0x2004) (read)", ppu.latch == 0x89);

    return 0;
}

char *test_write_2004(void) {
    ppu_init();

    mem_write(0x2003, 0x33);
    mem_write(0x2004, 0x55);
    ASSERT("OAMDATA (0x2004) (write)", ppu.oam[0x33] == 0x55);
    ASSERT("OAMADDR (0x2004) (write)", ppu.latch == 0x55);

    return 0;
}

char *test_write_2005(void) {
    ppu_init();
    ppu.w = false;

    /* First write. */
    mem_write(0x2005, 0x93);
    ASSERT("PPUSCROLL (0x2005) (write)", (ppu.t & 0x001F) == 0x12);
    ASSERT("PPUSCROLL (0x2005) (write)", ppu.x == 0x03);
    ASSERT("PPUSCROLL (0x2005) (write)", ppu.w);

    /* Second write. */
    mem_write(0x2005, 0x93);
    ASSERT("PPUSCROLL (0x2005) (write)", ((ppu.t & 0x03E0) >> 5) == 0x12);
    ASSERT("PPUSCROLL (0x2005) (write)", ((ppu.t & 0xF000) >> 12) == 0x03);
    ASSERT("PPUSCROLL (0x2005) (write)", !ppu.w);

    return 0;
}

char *test_write_2006(void) {
    ppu_init();
    ppu.w = false;
    ppu.t = 0x80;

    /* First write. */
    mem_write(0x2006, 0x69);
    ASSERT("PPUADDR (0x2006) (write)", ((ppu.t & 0xFF00) >> 8) == 0x29);
    ASSERT("PPUADDR (0x2006) (write)", ppu.w);

    /* Second write. */
    mem_write(0x2006, 0x89);
    ASSERT("PPUADDR (0x2006) (write)", (ppu.t & 0x00FF) == 0x89);
    ASSERT("PPUADDR (0x2006) (write)", ppu.v == 0x2989);
    ASSERT("PPUADDR (0x2006) (write)", !ppu.w);

    return 0;
}

char *test_read_2007(void) {
    ppu_init();
    ppu.ctrl_increment = 1;

    /* Test read before palettes. */
    mem_write(0x2006, 0x20);
    mem_write(0x2006, 0x22);
    vrm_write(0x2022, 0x89);
    mem_read(0x2007);   /* Discard the first result. */
    ASSERT("PPUDATA (0x2007) (read)", ppu.v == 0x2023);

    ppu.ctrl_increment = 32;
    ASSERT("PPUDATA (0x2007) (read)", mem_read(0x2007) == 0x89);
    ASSERT("PPUDATA (0x2007) (read)", ppu.v == 0x2043);

    /* Test read from palette. */
    ppu.ctrl_increment = 1;
    mem_write(0x2006, 0x3F);
    mem_write(0x2006, 0x01);
    vrm_write(0x3F01, 0x89);
    ASSERT("PPUDATA (0x2007) (read)", mem_read(0x2007) == 0x89);
    ASSERT("PPUDATA (0x2007) (read)", ppu.v == 0x3F02);

    return 0;
}

char *test_write_2007(void) {
    ppu_init();
    ppu.ctrl_increment = 32;

    mem_write(0x2006, 0x20);
    mem_write(0x2006, 0x22);
    mem_write(0x2007, 0x89);
    ASSERT("PPUDATA (0x2007) (write)", vrm_read(0x2022) == 0x89);
    ASSERT("PPUDATA (0x2007) (write)", ppu.v == 0x2042);

    return 0;
}

char *test_write_4014(void) {
    ppu_init();

    /* OAM address is zero. */
    for (int i = 0; i < 256; i++) {
        mem_write(0x0300 | i, i);
    }
    mem_write(0x2003, 0x00);
    mem_write(0x4014, 0x03);
    for (int i = 0; i < 256; i++) {
        ASSERT("OAMDMA (0x4014) (write)", ppu.oam[i] == i);
    }

    /* OAM address is offset. */
    for (int i = 0; i < 256; i++) {
        mem_write(0x0400 | i, i);
    }
    mem_write(0x2003, 0x11);
    mem_write(0x4014, 0x04);
    for (int i = 0; i < 256; i++) {
        ASSERT("OAMDMA (0x4014) (write)", ppu.oam[(i + 0x11) & 0xFF] == i);
    }

    return 0;
}

char *ppu_tests(void) {
    RUN_TEST( test_read_2002 );
    RUN_TEST( test_read_2004 );
    RUN_TEST( test_read_2007 );

    RUN_TEST( test_write_2000 );
    RUN_TEST( test_write_2001 );
    RUN_TEST( test_write_2003 );
    RUN_TEST( test_write_2004 );
    RUN_TEST( test_write_2005 );
    RUN_TEST( test_write_2006 );
    RUN_TEST( test_write_2007 );
    RUN_TEST( test_write_4014 );

    return 0;
}
