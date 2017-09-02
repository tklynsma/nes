#include <stdio.h>
#include <stdbool.h>
#include "../include/cpu.h"
#include "../include/memory.h"
#include "../include/ppu.h"
#include "../include/vram.h"

/* -----------------------------------------------------------------
 * PPU status.
 * -------------------------------------------------------------- */

typedef struct {
    /* PPU storage. */
    byte nametable[2048];       /* PPU nametables. */
    byte palette[32];           /* PPU palettes */
    byte oam[256];              /* Object attribute memory. */

    /* PPU internal registers. */
    word v;                     /* Current VRAM address (15 bit). */
    word t;                     /* Temporary VRAM address (15 bit). */
    byte x;                     /* Fine X scroll (3 bits). */
    bool w;                     /* First or second write toggle. */

    /* 0x2000: PPU control register 1. */
    bool ctrl_nmi;              /* Generate an NMI at start of VBI */
    bool ctrl_master_slave;     /* PPU master/slave select. */
    bool ctrl_sprite_size;      /* Sprite size (0: 8x8; 1: 8x16). */
    word ctrl_background_addr;  /* Background pattern table address. */
    word ctrl_sprite_addr;      /* Sprite pattern table address. */
    byte ctrl_increment;        /* VRAM address increment. */
    word ctrl_nametable_addr;   /* Base nametable address. */

    /* 0x2001: PPU control register 2. */
    bool mask_red;              /* Emphasize red (NTSC) or green (PAL). */
    bool mask_green;            /* Emphasize green (NTSC) or red (PAL). */
    bool mask_blue;             /* Emphasize blue. */
    bool mask_sprites;          /* Render sprites. */
    bool mask_background;       /* Render background. */
    bool mask_sprites_L;        /* Render sprites in leftmost column. */
    bool mask_background_L;     /* Render background in leftmost column. */
    bool mask_grayscale;        /* Produce a grayscale display. */

    /* 0x2002: PPU status register. */
    bool status_vblank;         /* Vertical blank has started. */
    bool status_zero_hit;       /* Set when sprite 0 overlaps with bg pixel. */
    bool status_overflow;       /* Set when more than 8 sprites on scanline. */

    /* Other. */
    byte oam_addr;              /* 0x2003: OAM address. */
    byte vram_buffer;           /* Internal read buffer. */
    byte latch;                 /* PPUGenLatch. */
} PPU;

static PPU ppu;

/* -----------------------------------------------------------------
 * PPU read/write.
 * -------------------------------------------------------------- */

static inline byte read_ppu_status() {
    ppu.latch &= 0x1F;
    ppu.latch |= (ppu.status_vblank   << 7);
    ppu.latch |= (ppu.status_zero_hit << 6);
    ppu.latch |= (ppu.status_overflow << 5);
    ppu.status_vblank = false;
    ppu.w = false;
    return ppu.latch;
}

static inline byte read_oam_data() {
    ppu.latch = ppu.oam[ppu.oam_addr];
    return ppu.latch;
}

static inline byte read_ppu_data() {
    if (ppu.v % 0x4000 < 0x3F00) {
        ppu.latch = ppu.vram_buffer;
        ppu.vram_buffer = vrm_read(ppu.v);
    }
    else {
        ppu.latch = vrm_read(ppu.v);
        ppu.vram_buffer = vrm_read(ppu.v - 0x1000);
    }

    ppu.v += ppu.ctrl_increment;
    return ppu.latch;
}

static inline void write_ppu_ctrl(byte data) {
    ppu.ctrl_nmi             = data & 0x80;
    ppu.ctrl_master_slave    = data & 0x40;
    ppu.ctrl_sprite_size     = data & 0x20;
    ppu.ctrl_background_addr = data & 0x10 ? 0x1000 : 0x0000;
    ppu.ctrl_sprite_addr     = data & 0x08 ? 0x1000 : 0x0000;
    ppu.ctrl_increment       = data & 0x04 ? 32 : 1;
    ppu.ctrl_nametable_addr  = 0x2000 + 0x400 * (data & 0x03);
    ppu.t = (ppu.t & 0x73FF) | ((data & 0x3) << 10);
}

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

static inline void write_oam_address(byte data) {
    ppu.oam_addr = data; 
}

static inline void write_oam_data(byte data) {
    ppu.oam[ppu.oam_addr++] = data;
}

static inline void write_ppu_scroll(byte data) {
    if (ppu.w) {
        ppu.t = (ppu.t & 0x8FFF) | ((data & 0x07) << 12);
        ppu.t = (ppu.t & 0xFC1F) | ((data & 0xF8) << 2);
        ppu.w = false;
    } else {
        ppu.t = (ppu.t & 0xFFE0) | (data >> 3);
        ppu.x = data & 0x07;
        ppu.w = true;
    }
}

static inline void write_ppu_address(byte data) {
    if (ppu.w) {
        ppu.t = (ppu.t & 0xFF00) | data;
        ppu.v = ppu.t;
        ppu.w = false;
    }
    else {
        ppu.t = (ppu.t & 0x00FF) | ((data & 0x3F) << 8);
        ppu.w = true;
    }
}

static inline void write_ppu_data(byte data) {
    vrm_write(ppu.v, data);
    ppu.v += ppu.ctrl_increment;
}

inline byte ppu_io_read(word address) {
    switch (address & 0x7) {
        case 2: return read_ppu_status();
        case 4: return read_oam_data();
        case 7: return read_ppu_data();
    }
    return ppu.latch;
}

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

inline byte ppu_dma_read(void) {
    return ppu.latch;
}

inline void ppu_dma_write(byte data) {
    word address = data << 8;
    for (int i = 0; i < 256; i++) {
        ppu.oam[ppu.oam_addr++] = mem_read(address++);
    }
    cpu_suspend(513 + (cpu_get_ticks() % 2));
    ppu.latch = data;
}

inline byte ppu_palette_read(word address) {
    return ppu.palette[address];
}

inline void ppu_palette_write(word address, byte data) {
    ppu.palette[address] = data;
}

inline byte ppu_nametable_read(word address) {
    return ppu.nametable[address];
}

inline void ppu_nametable_write(word address, byte data) {
    ppu.nametable[address] = data;
}

/* -----------------------------------------------------------------
 * Initial setup of PPU rendering (simplified).
 * -------------------------------------------------------------- */

static inline void render_background_tile(byte display[256][240], byte tile, int row, int col) {
    word pattern_table_addr = ppu.ctrl_background_addr | (tile << 8);
    for (int i = 0; i < 8; i++) {
        byte bit_pattern_0 = vrm_read(pattern_table_addr);
        byte bit_pattern_1 = vrm_read(pattern_table_addr + 0x8);
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
            byte tile = vrm_read(ppu.ctrl_nametable_addr++);
            render_background_tile(display, tile, row, col);
        }
    }
}

void ppu_render_frame(byte display[256][240]) {
    render_background(display);
}
