#include <stdio.h>
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

byte display[256][240];

static inline bool is_rendering_background(void) {
    return ppu.mask_background;
}

static inline bool is_rendering_sprites(void) {
    return ppu.mask_sprites;
}

static inline bool is_rendering(void) {
    return ppu.mask_background || ppu.mask_sprites;
}

static inline bool is_prerender_line(void) {
    return ppu.scanline == -1;
}

static inline bool is_visible_line(void) {
    return ppu.scanline >= 0 && ppu.scanline < 240;
}

static inline bool is_render_line(void) {
    return ppu.scanline < 240;
}

/* -----------------------------------------------------------------
 * Horizontal / vertical increments.
 * -------------------------------------------------------------- */

/* Coarse X increment. */
static inline void increment_x(void) {
    if ((ppu.v & 0x001F) == 31) {
        ppu.v &= ~0x001F;
        ppu.v ^= 0x0400;
    } else {
        ppu.v += 1;
    }
}

/* Y increment. */
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

/* V increment after 0x2007 access. */
static inline void increment_v(void) {
    if (is_rendering() && is_render_line()) {
        increment_x();
        increment_y();
    }
    else {
        ppu.v += ppu.ctrl_increment;
    }
}

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

/* 0x2002: PPUSTATUS (get). */
static inline byte get_ppu_status() {
    byte result = ppu.latch & 0x1F;
    result |= (ppu.status_vblank   << 7);
    result |= (ppu.status_zero_hit << 6);
    result |= (ppu.status_overflow << 5);
    return result;
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

/* 0x2004: OAMDATA (get). */
static inline byte get_oam_data() {
    return ppu.oam[ppu.oam_addr];
}

/* 0x2004: OAMDATA (write). */
static inline void write_oam_data(byte data) {
    ppu.oam[ppu.oam_addr++] = data;
}

/* 0x2005: PPUSCROLL (write). */
static inline void write_ppu_scroll(byte data) {
    /* First write (w is 0). */ 
    if (!ppu.w) {
        /* t: ....... ...HGFED = d: HGFED... *
         * x:              CBA = d: .....CBA *
         * w:                  = 1           */
        ppu.t = (ppu.t & 0xFFE0) | (data >> 3);
        ppu.x = data & 0x07;
        ppu.w = true;
    }
    /* Second write (w is 1). */
    else {
        /* t: CBA..HG FED..... = d: HGFEDCBA *
         * w:                  = 0           */
        ppu.t = (ppu.t & 0x8FFF) | ((data & 0x07) << 12);
        ppu.t = (ppu.t & 0xFC1F) | ((data & 0xF8) << 2);
        ppu.w = false;
    }
}

/* 0x2006: PPUADDR (write). */
static inline void write_ppu_address(byte data) {
    /* First write (w is 0). */
    if (!ppu.w) {
        /* t: .FEDCBA ........ = d: ..FEDCBA *
         * t: X...... ........ = 0           *
         * w:                  = 1           */
        ppu.t = (ppu.t & 0x80FF) | ((data & 0x3F) << 8);
        ppu.w = true;
    }
    /* Second write (w is 1). */
    else {
        /* t: ....... HGFEDCBA = d: HGFEDCBA *
         * v                   = t           *
         * w:                  = 0           */
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
        ppu.latch = ppu_palette_read(ppu.v);
        ppu.read_buffer = vrm_read(ppu.v - 0x1000);
    }

    increment_v();
    return ppu.latch;
}

/* 0x2007: PPUDATA (get). */
static inline byte get_ppu_data() {
    return (ppu.v & 0x3FFF) < 0x3F00 ? ppu.read_buffer : ppu_palette_read(ppu.v);
}

/* 0x2007: PPUDATA (write). */
static inline void write_ppu_data(byte data) {
    vrm_write(ppu.v, data);
    increment_v();
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

/* 0x2000-0x2007: Read PPU register. */
inline byte ppu_io_read(word address) {
    switch (address & 0x7) {
        case 2: return read_ppu_status();
        case 4: return read_oam_data();
        case 7: return read_ppu_data();
    }
    return ppu.latch;
}

/* 0x2000-0x2007: Read PPU register (without side-effects). */
inline byte ppu_io_get(word address) {
    switch (address & 0x7) {
        case 2: return get_ppu_status();
        case 4: return get_oam_data();
        case 7: return get_ppu_data();
    }
    return ppu.latch;
}

/* 0x2000-0x2007: Write PPU register. */
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

/* 0x3F00-0x3FFF: Read PPU palette. */
inline byte ppu_palette_read(word address) {
    return ppu.palette[address & 0x1F];
}

/* 0x3F00-0x3FFF: Write PPU palette. */
inline void ppu_palette_write(word address, byte data) {
    ppu.palette[address & 0x1F] = data;
}

/* 0x2000-0x3EFF: Read PPU nametable. */
inline byte ppu_nametable_read(word address) {
    return ppu.nametable[address & 0x1FFF];
}

/* 0x2000-0x3EFF: Write PPU nametable. */
inline void ppu_nametable_write(word address, byte data) {
    ppu.nametable[address & 0x1FFF] = data;
}

/* -----------------------------------------------------------------
 * PPU rendering.
 * -------------------------------------------------------------- */

static inline void copy_horizontal(void) {
    /* v: ....F.. ...EDCBA = t: ....F.. ...EDCBA */
    ppu.v = (ppu.v & 0xFBE0) | (ppu.t & ~0xFBE0);
}

static inline void copy_vertical(void) {
    /* v: IHGF.ED CBA..... = t: IHGF.ED CBA..... */
    ppu.v = (ppu.v & 0x841F) | (ppu.t & ~0x841F);
}

static inline void fetch_nametable_byte(void) {
    ppu.nametable_byte = vrm_read(0x2000 | (ppu.v & 0x0FFF));
}

static inline void fetch_attribute_byte(void) {
    word address = 0x23C0 | (ppu.v & 0x0C00);
    address = address | ((ppu.v >> 4) & 0x38);
    address = address | ((ppu.v >> 2) & 0x07);
    ppu.attribute_byte = vrm_read(address);
}

static inline void fetch_low_tile(void) {
    byte fine_y = (ppu.v >> 12) & 0x7;
    word address = ppu.ctrl_background_addr + 16 * ppu.nametable_byte + fine_y;
    ppu.low_tile = vrm_read(address);
}

static inline void fetch_high_tile(void) {
    byte fine_y = (ppu.v >> 12) & 0x7;
    word address = ppu.ctrl_background_addr + 16 * ppu.nametable_byte + fine_y;
    ppu.high_tile = vrm_read(address + 8);
}

/* Store the background tile data in the shift registers. */
static inline void store_tile_data(void) {
    ppu.low_tile_register  |= (ppu.low_tile  << 8);
    ppu.high_tile_register |= (ppu.high_tile << 8);
}

/* Update the scanline and dot counters after every cycle. */
void ppu_tick(void) {
    ppu.dot++;
    if (ppu.dot > 340) {
        ppu.dot = 0;

        ppu.scanline++;
        if (ppu.scanline > 260) {
            ppu.scanline = -1;

            /* Skip the first pre-render cycle on odd frames. */
            ppu.odd_frame = !ppu.odd_frame;
            if (ppu.odd_frame && is_rendering()) {
                ppu.dot = 1;
            }
        }
    }
}

void render_dot(void) {
    int x = ppu.dot - 1, y = ppu.scanline;

    if (ppu.dot - 1 < 8 && !ppu.mask_background_L) {
        display[x][y] = 0x00;
    }
    else {
        byte bit_0 = (ppu.low_tile_register  >> ppu.x) & 0x1;
        byte bit_1 = (ppu.high_tile_register >> ppu.x) & 0x1;
        display[x][y] = (bit_1 << 1) | bit_0;
    }
}

void ppu_step(void) {
    if (is_rendering()) {
        /* Pre-render scanline (261). */
        if (is_prerender_line()) {
            /* During dots 280 to 340 of the pre-render scanline: If rendering
            * if enabled, the PPU copies all bits related to vertical position
            * from t to v. */
            if (ppu.dot >= 280 && ppu.dot <= 340) {
                copy_vertical();
            }
        }

        /* Visible dots (1-256) in visible scanlines (0-239). */
        if (is_visible_line() && ppu.dot > 0 && ppu.dot <= 256) {
            render_dot();
        }

        /* Render scanlines (0-239, 261). */
        if (is_visible_line() || is_prerender_line()) {
            /* During dots 1 to 256 and dots 321 to 336: the data for each tile is
            * fetched. Every 8 dots the horizontal position in v is incremented and
            * the tile data is stored in the shift registers. */
            if ((ppu.dot > 0 && ppu.dot < 257) || (ppu.dot > 320 && ppu.dot < 337)) {
                ppu.low_tile_register  >>= 1;
                ppu.high_tile_register >>= 1;

                switch (ppu.dot % 8) {
                    case 1: fetch_nametable_byte(); break;
                    case 3: fetch_attribute_byte(); break;
                    case 5: fetch_low_tile();       break;
                    case 7: fetch_high_tile();      break;
                    case 0: store_tile_data();
                            increment_x();          break;
                }
            }

            /* At dot 256 of each scanline: If rendering is enabled, the PPU
            * increments the vertical position in v. */
            if (ppu.dot == 256) {
                increment_y();
            }

            /* At dot 257 of each scanline: If rendering is enabled, the PPU
            * copies all bits related to horizontal position from t to v. */
            else if (ppu.dot == 257) {
                copy_horizontal();
            }
        }
    }

    /* Start of vblank (scanline 241, dot 1). */
    if (ppu.scanline == 241 && ppu.dot == 1) {
        ppu.status_vblank = true;
        if (ppu.ctrl_nmi) {
            cpu_set_nmi();
        }
    }
    
    /* End of vblank (scanline 261, dot 1). */ 
    if (is_prerender_line() && ppu.dot == 1) {
        ppu.status_vblank   = false;
        ppu.status_zero_hit = false;
        ppu.status_overflow = false;
    }

    ppu_tick();
}

void ppu_cycle(int num_cycles) {
    while (num_cycles > 0) {
        ppu_step();
        num_cycles--;
    }
}

inline byte ppu_get_pixel(int x, int y) {
    return display[x][y];
}

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

    ppu.dot      =  0;
    ppu.scanline = -1;
}

void ppu_reset(void) {
    write_ppu_ctrl(0x00);
    write_ppu_mask(0x00);

    ppu.latch = ppu.read_buffer = 0x00;

    ppu.x = 0x00;
    ppu.t = 0x0000;
    ppu.w = false;
}
