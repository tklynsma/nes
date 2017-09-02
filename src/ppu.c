#include <stdbool.h>
#include "../include/cpu.h"
#include "../include/memory.h"
#include "../include/ppu.h"
#include "../include/ppu_internal.h"
#include "../include/vram.h"

/* -----------------------------------------------------------------
 * PPU status.
 * -------------------------------------------------------------- */

PPU ppu;

static int scanline = 0;
static int dot = 0;

/* -----------------------------------------------------------------
 * PPU read/write.
 * -------------------------------------------------------------- */

/* 0x2000: PPUCTRL (write). */
static inline void write_ppu_ctrl(byte data) {
    ppu.ctrl_nmi             = data & 0x80;
    ppu.ctrl_master_slave    = data & 0x40;
    ppu.ctrl_sprite_size     = data & 0x20;
    ppu.ctrl_background_addr = data & 0x10 ? 0x1000 : 0x0000;
    ppu.ctrl_sprite_addr     = data & 0x08 ? 0x1000 : 0x0000;
    ppu.ctrl_increment       = data & 0x04 ? 32 : 1;

    /* t: ...BA.. ........ = d: ......BA */
    ppu.t = (ppu.t & 0xF3FF) | ((data & 0x3) << 10);
}

/* 0x2001: PPUMASK (write). */
static inline void write_ppu_mask(byte data) {
    ppu.mask_red             = data & 0x80;
    ppu.mask_green           = data & 0x40;
    ppu.mask_blue            = data & 0x20;
    ppu.mask_sprites         = data & 0x10;
    ppu.mask_background      = data & 0x08;
    ppu.mask_sprites_L       = data & 0x04;
    ppu.mask_background_L    = data & 0x02;
    ppu.mask_grayscale       = data & 0x01;
}

/* 0x2002: PPUSTATUS (read). */
static inline byte read_ppu_status() {
    ppu.latch &= 0x1F;
    ppu.latch |= (ppu.status_vblank   << 7);
    ppu.latch |= (ppu.status_zero_hit << 6);
    ppu.latch |= (ppu.status_overflow << 5);
    ppu.status_vblank = false;
    ppu.w = false;           /* w:    = 0 */
    return ppu.latch;
}

/* 0x2003: OAMADDR (write). */
static inline void write_oam_address(byte data) {
    ppu.oam_addr = data; 
}

/* 0x2004: OAMDATA (read). */
static inline byte read_oam_data() {
    ppu.latch = ppu.oam[ppu.oam_addr];
    return ppu.latch;
}

/* 0x2004: OAMDATA (write). */
static inline void write_oam_data(byte data) {
    ppu.oam[ppu.oam_addr++] = data;
}

/* 0x2005: PPUSCROLL (write). */
static inline void write_ppu_scroll(byte data) {
    /* First write (w is 0). */ 
    if (!ppu.w) {
        /* t: ....... ...HGFED = d: HGFED... */
        /* x:              CBA = d: .....CBA */
        /* w:                  = 1           */
        ppu.t = (ppu.t & 0xFFE0) | (data >> 3);
        ppu.x = data & 0x07;
        ppu.w = true;
    }
    /* Second write (w is 1). */
    else {
        /* t: CBA..HG FED..... = d: HGFEDCBA */
        /* w:                  = 0           */
        ppu.t = (ppu.t & 0x8FFF) | ((data & 0x07) << 12);
        ppu.t = (ppu.t & 0xFC1F) | ((data & 0xF8) << 2);
        ppu.w = false;
    }
}

/* 0x2006: PPUADDR (write). */
static inline void write_ppu_address(byte data) {
    /* First write (w is 0). */
    if (!ppu.w) {
        /* t: .FEDCBA ........ = d: ..FEDCBA */
        /* t: X...... ........ = 0           */
        /* w:                  = 1           */
        ppu.t = (ppu.t & 0x00FF) | ((data & 0x3F) << 8);
        ppu.w = true;
    }
    /* Second write (w is 1). */
    else {
        /* t: ....... HGFEDCBA = d: HGFEDCBA */
        /* v                   = t           */
        /* w:                  = 0           */
        ppu.t = (ppu.t & 0xFF00) | data;
        ppu.v = ppu.t;
        ppu.w = false;
    }
}

/* 0x2007: PPUDATA (read). */
static inline byte read_ppu_data() {
    if ((ppu.v & 0x3FFF) < 0x3F00) {
        ppu.latch = ppu.read_buffer;
        ppu.read_buffer = vrm_read(ppu.v);
    }
    else {
        ppu.latch = vrm_read(ppu.v);
        ppu.read_buffer = vrm_read(ppu.v - 0x1000);
    }

    ppu.v += ppu.ctrl_increment;
    return ppu.latch;
}

/* 0x2007: PPUDATA (write). */
static inline void write_ppu_data(byte data) {
    vrm_write(ppu.v, data);
    ppu.v += ppu.ctrl_increment;
}

/* 0x4014: OAMDMA (read). */
inline byte ppu_dma_read(void) {
    return ppu.latch;
}

/* 0x4014: OAMDMA (write). */
inline void ppu_dma_write(byte data) {
    word mem_address = data << 8;
    for (int i = 0; i < 256; i++) {
        ppu.oam[ppu.oam_addr++] = mem_read(mem_address++);
    }
    cpu_suspend(513 + (cpu_get_ticks() % 2));
    ppu.latch = data;
}

/* Read PPU register (0x2000 - 0x2007). */
inline byte ppu_io_read(word address) {
    switch (address & 0x7) {
        case 2: return read_ppu_status();
        case 4: return read_oam_data();
        case 7: return read_ppu_data();
    }
    return ppu.latch;
}

/* Write PPU register (0x2000 - 0x2007). */
inline void ppu_io_write(word address, byte data) {
    switch (address & 0x7) {
        case 0: write_ppu_ctrl(data);      break;
        case 1: write_ppu_mask(data);      break;
        case 3: write_oam_address(data);   break;
        case 4: write_oam_data(data);      break;
        case 5: write_ppu_scroll(data);    break;
        case 6: write_ppu_address(data);   break;
        case 7: write_ppu_data(data);      break;
    }
    ppu.latch = data;
}

/* Read PPU palette (0x3F00 - 0x3FFF). */
inline byte ppu_palette_read(word address) {
    return ppu.palette[address & 0x1F];
}

/* Write PPU palette (0x3F00 - 0x3FFF). */
inline void ppu_palette_write(word address, byte data) {
    ppu.palette[address & 0x1F] = data;
}

/* Read PPU nametable (0x2000 - 0x3EFF). */
inline byte ppu_nametable_read(word address) {
    return ppu.nametable[address & 0x1FFF];
}

/* Write PPU nametable (0x2000 - 0x3EFF). */
inline void ppu_nametable_write(word address, byte data) {
    ppu.nametable[address & 0x1FFF] = data;
}

/* -----------------------------------------------------------------
 * PPU rendering.
 * -------------------------------------------------------------- */

static inline byte fetch_nametable_byte(void) {
    return vrm_read(0x2000 | (ppu.v & 0x0FFF));
}

static inline byte fetch_attribute_byte(void) {
    word address = 0x23C0 | (ppu.v & 0x0C00) | ((ppu.v >> 4) & 0x38) | ((ppu.v >> 2) & 0x07);
    return vrm_read(address);
}

static inline void increment_x(void) {
    if ((ppu.v & 0x001F) == 31) {
        ppu.v &= ~0x001F;
        ppu.v ^= 0x0400;
    } else {
        ppu.v += 1;
    }
}

static inline void increment_y(void) {
    /* If fine Y < 7 then increment fine Y. */
    if ((ppu.v & 0x7000) != 0x7000) {
        ppu.v += 0x1000;
    }

    /* Otherwise, update coarse Y. */
    else {
        ppu.v &= ~0x7000;
        byte coarse_y = (ppu.v & 0x03E0) >> 5;

        if (coarse_y == 29) {
            coarse_y = 0;
            ppu.v ^= 0x0800;
        }
        else if (coarse_y == 31) {
            coarse_y = 0;
        }
        else {
            coarse_y += 1;
        }
        ppu.v = (ppu.v & ~0x03E0) | (coarse_y << 5);
    }
}

/*static inline void render_background_tile(byte display[256][240], byte tile, int row, int col) {
    word pattern_table_addr = ppu.ctrl_background_addr | (tile << 8);
    word pattern_table_addr = ppu.ctrl_background_addr | (tile << 4) | ((ppu.v >> 12) & 7);
    for (int i = 0; i < 8; i++) {
        byte bit_pattern_0 = vrm_read(pattern_table_addr);
        byte bit_pattern_1 = vrm_read(pattern_table_addr | 8);
        for (int j = 0; j < 8; j++) {
            bool bit_0 = bit_pattern_0 & (0x1 << j);
            bool bit_1 = bit_pattern_1 & (0x1 << j);
            display[8 * col + j][8 * row + i] = (bit_1 << 1) | bit_0;
        }
        pattern_table_addr++;
    }
}

static inline void render_background(byte display[256][240]) {
    for (int row = 0; row < 30; row++) {
        for (int col = 0; col < 32; col++) {
            byte tile = fetch_nametable_byte();
            render_background_tile(display, tile, row, col);
            increment_coarse_x();
        }
    }
}

void ppu_render_frame(byte display[256][240]) {
    render_background(display);
}*/

void ppu_render_frame(byte display[256][240]) {}

/* -----------------------------------------------------------------
 * Initialize/Reset PPU.
 * -------------------------------------------------------------- */

void ppu_init(void) {
    write_ppu_ctrl   (0x00);
    write_ppu_mask   (0x00);
    write_oam_address(0x00);

    ppu.status_vblank   = true;
    ppu.status_zero_hit = false;
    ppu.status_overflow = true;

    ppu.latch = ppu.read_buffer = 0x00;

    ppu.x = 0x00;
    ppu.v = 0x0000;
    ppu.t = 0x0000;
    ppu.w = false;

    for (int i = 0; i < NAMETABLE_SIZE; i++) {
        ppu.nametable[i] = 0xFF;
    }
}

void ppu_reset(void) {
    write_ppu_ctrl(0x00);
    write_ppu_mask(0x00);

    ppu.latch = ppu.read_buffer = 0x00;

    ppu.x = 0x00;
    ppu.t = 0x0000;
    ppu.w = false;
}
