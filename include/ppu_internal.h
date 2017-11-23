#ifndef PPU_INTERNAL_H
#define PPU_INTERNAL_H

#define NAMETABLE_SIZE 2048
#define PALETTE_SIZE   32
#define OAM_SIZE       256
#define MAX_SPRITES    8

#include <stdbool.h>
#include "common.h"

typedef struct {
    byte x, y;                      /* Top-left location of the sprite. */
    byte tile;                      /* Sprite byte 1: Tile index number. */
    byte low_tile;                  /* Low tile data for the current scanline. */
    byte high_tile;                 /* High tile data for the current scanline. */
    bool visible;                   /* Display the sprite. */

    /* Sprite byte 2: Attributes. */
    byte palette;                   /* Palette of sprite. */
    bool priority;                  /* Priority (0: front, 1: back). */
    bool flip_h;                    /* Flip sprite horizontally. */
    bool flip_v;                    /* Flip sprite vertically. */
} Sprite;

typedef struct {
    byte pixel;                     /* Pixel data (2 bit). */
    byte palette;                   /* Palette data (2 bit). */
    bool priority;                  /* Priority (0: front, 1: back). */
} Pixel;

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
    byte ctrl_sprite_size;          /* Sprite height (8 or 16 pixels). */
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

    /* PPU Rendering. */
    int scanline;                   /* [0, 261]: 262 scanlines per frame. */
    int dot;                        /* [0, 340]: 341 cycles per scanline. */
    bool odd_frame;                 /* The current frame is an odd frame. */

    /* Background rendering. */
    byte nametable_byte;            /* The current nametable byte being fetched. */
    byte attribute_byte;            /* The current attribute byte being fetched. */
    byte low_tile;                  /* The current tile low pattern being fetched. */
    byte high_tile;                 /* The current tile high pattern being fetched. */

    word low_tile_register;         /* Low tile shift register (16 bit). */
    word high_tile_register;        /* High tile shift register (16 bit). */
    byte attribute_register;        /* Attribute shift register (4 bits). */

    /* Sprite rendering. */
    Sprite sprites[8];              /* Sprites data of the current scanline. */
    byte sprite_count;              /* Current sprite. */
} PPU;

extern PPU ppu;

#endif /* PPU_INTERNAL_H */
