#include <stdbool.h>
#include "../include/cpu.h"
#include "../include/memory.h"
#include "../include/ppu.h"
#include "../include/vram.h"

/* -----------------------------------------------------------------
 * PPU status.
 * -------------------------------------------------------------- */

typedef struct {
    bool ctrl_nmi;             /* Generate an NMI at the start of the VBI */
    bool ctrl_master_slave;    /* PPU master/slave select. */
    bool ctrl_sprite_size;     /* Sprite size (0: 8x8; 1: 8x16). */
    bool ctrl_background_addr; /* Background pattern table address (high bit). */
    bool ctrl_sprite_addr;     /* Sprite pattern table address for 8x8 sprites (high bit). */
    byte ctrl_increment;       /* VRAM address increment per CPU read/write of PPUDATA (1 or 32). */
    byte ctrl_nametable_addr;  /* Base nametable address (0: 0x2000; 1: 0x2400; 2: 0x2800; 3: 0x2C00). */

    bool mask_red;             /* Emphasize red (NTSC) or green (PAL). */
    bool mask_green;           /* Emphasize green (NTSC) or red (PAL). */
    bool mask_blue;            /* Emphasize blue. */
    bool mask_sprites;         /* Render sprites. */
    bool mask_background;      /* Render background. */
    bool mask_sprites_L;       /* Render sprites in the leftmost column. */
    bool mask_background_L;    /* Render background in the leftmost column. */
    bool mask_grayscale;       /* Produce a grayscale display. */

    bool stat_vblank;          /* Vertical blank has started. */
    bool stat_sprite_hit;      /* Set when sprite 0 overlaps with a background pixel. */
    bool stat_sprite_overflow; /* Set when more than 8 sprites appear on a scanline (buggy). */

    byte oam[OAM_SIZE];        /* OAM (Object Attribute Memory). */
    byte oam_addr;             /* OAM address (0x2003). */
    byte oam_data;             /* OAM data (0x2004). */

    word vram_addr;            /* PPU address register (0x2006). */
    byte vram_data;            /* PPU data (0x2007). */
    byte vram_buffer;          /* Internal read buffer. */

    byte scroll_x;             /* Fine scroll position (X) (0x2005). */
    byte scroll_y;             /* Fine scroll position (Y) (0x2005). */

    byte latch;                /* PPUGenLatch. */
    bool write_toggle;         /* ... */
} PPU;

static PPU ppu;

/* -----------------------------------------------------------------
 * PPU read/write registers.
 * -------------------------------------------------------------- */

static inline byte read_ppu_status() {
    ppu.latch &= 0x1F;
    ppu.latch |= (ppu.stat_vblank          << 7);
    ppu.latch |= (ppu.stat_sprite_hit      << 6);
    ppu.latch |= (ppu.stat_sprite_overflow << 5);
    ppu.stat_vblank = false;
    ppu.write_toggle = false;
    return ppu.latch;
}

static inline byte read_oam_data() {
    ppu.latch = ppu.oam[ppu.oam_addr];
    return ppu.latch;
}

static inline byte read_ppu_data() {
    if (ppu.vram_addr % 0x4000 < 0x3F00) {
        ppu.latch = ppu.vram_buffer;
        ppu.vram_buffer = vrm_read(ppu.vram_addr);
    }
    else {
        ppu.latch = vrm_read(ppu.vram_addr);
        ppu.vram_buffer = vrm_read(ppu.vram_addr - 0x1000);
    }

    ppu.vram_addr += ppu.ctrl_increment;
    return ppu.latch;
}

static inline void write_ppu_ctrl(byte data) {
    ppu.ctrl_nmi             = data & 0x80;
    ppu.ctrl_master_slave    = data & 0x40;
    ppu.ctrl_sprite_size     = data & 0x20;
    ppu.ctrl_background_addr = data & 0x10;
    ppu.ctrl_sprite_addr     = data & 0x08;
    ppu.ctrl_increment       = data & 0x04 ? 32 : 1;
    ppu.ctrl_nametable_addr  = data & 0x03;
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
    ppu.oam_data = ppu.oam[ppu.oam_addr++] = data;
}

static inline void write_ppu_scroll(byte data) {
    if (ppu.write_toggle) {
        ppu.scroll_y = data;
    } else {
        ppu.scroll_x = data;
    }
    ppu.write_toggle = !ppu.write_toggle;
}

static inline void write_ppu_address(byte data) {
    if (ppu.write_toggle) {
        ppu.vram_addr = (ppu.latch << 8) | data;
    }
    ppu.write_toggle = !ppu.write_toggle;
}

static inline void write_ppu_data(byte data) {
    vrm_write(ppu.vram_addr, data);
    ppu.vram_addr += ppu.ctrl_increment;
}

static inline void write_oam_dma(byte data) {
    word address = data << 8;
    for (int i = 0; i < OAM_SIZE; i++) {
        ppu.oam[ppu.oam_addr++] = mem_read_8(address++);
    }
    cpu_suspend(513 + (cpu_get_ticks() % 2));
}

inline byte ppu_io_read(word address) {
    address = address < 0x4000 ? 0x2000 + (address % 8) : address;
    switch (address) {
        case 0x2002: return read_ppu_status();
        case 0x2004: return read_oam_data();
        case 0x2007: return read_ppu_data();
    }
    return ppu.latch;
}

inline void ppu_io_write(word address, byte data) {
    address = address < 0x4000 ? 0x2000 + (address % 8) : address;
    switch (address) {
        case 0x2000: write_ppu_ctrl(data);      break;
        case 0x2001: write_ppu_mask(data);      break;
        case 0x2003: write_oam_address(data);   break;
        case 0x2004: write_oam_data(data);      break;
        case 0x2005: write_ppu_scroll(data);    break;
        case 0x2006: write_ppu_address(data);   break;
        case 0x2007: write_ppu_data(data);      break;
        case 0x4014: write_oam_dma(data);       break;
    }
    ppu.latch = data;
}
