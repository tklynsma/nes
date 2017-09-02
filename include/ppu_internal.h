#ifndef PPU_INTERNAL_H
#define PPU_INTERNAL_H

#define NAMETABLE_SIZE 2048
#define PALETTE_SIZE   32
#define OAM_SIZE       256

#include <stdbool.h>
#include "common.h"

typedef struct {
    /* PPU storage. */
    byte nametable[NAMETABLE_SIZE]; /* PPU nametables. */
    byte palette[PALETTE_SIZE];     /* PPU palettes */
    byte oam[OAM_SIZE];             /* Object attribute memory. */

    /* PPU internal registers. */
    word v;                         /* Current VRAM address (15 bit). */
    word t;                         /* Temporary VRAM address (15 bit). */
    byte x;                         /* Fine X scroll (3 bits). */
    bool w;                         /* First or second write toggle. */

    /* 0x2000: PPU control register 1. */
    bool ctrl_nmi;                  /* Generate an NMI at start of VBI */
    bool ctrl_master_slave;         /* PPU master/slave select. */
    bool ctrl_sprite_size;          /* Sprite size (0: 8x8; 1: 8x16). */
    word ctrl_background_addr;      /* Background pattern table address. */
    word ctrl_sprite_addr;          /* Sprite pattern table address. */
    byte ctrl_increment;            /* VRAM address increment. */

    /* 0x2001: PPU control register 2. */
    bool mask_red;                  /* Emphasize red (NTSC) or green (PAL). */
    bool mask_green;                /* Emphasize green (NTSC) or red (PAL). */
    bool mask_blue;                 /* Emphasize blue. */
    bool mask_sprites;              /* Render sprites. */
    bool mask_background;           /* Render background. */
    bool mask_sprites_L;            /* Render sprites in leftmost column. */
    bool mask_background_L;         /* Render background in leftmost column. */
    bool mask_grayscale;            /* Produce a grayscale display. */

    /* 0x2002: PPU status register. */
    bool status_vblank;             /* Vertical blank has started. */
    bool status_zero_hit;           /* Set when sprite 0 overlaps with bg pixel. */
    bool status_overflow;           /* Set when more than 8 sprites on scanline. */

    /* Other. */
    byte oam_addr;                  /* 0x2003: OAM address. */
    byte read_buffer;               /* Internal read buffer. */
    byte latch;                     /* PPUGenLatch. */
} PPU;

extern PPU ppu;

#endif /* PPU_INTERNAL_H */
