#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <stdbool.h>
#include "../include/common.h"

#define INES_HEADER_SIZE  16
#define INES_TRAINER_SIZE 512
#define PRG_BANK_SIZE     16384
#define CHR_BANK_SIZE     8192

typedef struct {
    byte *prg_rom;      /* PRG ROM data. */
    byte *prg_ram;      /* PRG RAM data. */
    byte *chr_rom;      /* CHR ROM data. */
    byte *chr_ram;      /* CHR RAM data. */
    byte *registers;    /* Registers. */

    byte mapper;        /* Mapper number. */
    byte prg_banks;     /* Number of PRG ROM banks. */
    byte chr_banks;     /* Number of CHR ROM banks. */
    bool mirroring;     /* Mirroring (0: horizontal; 1: vertical). */
    bool sram;          /* Contains battery-backed PRG RAM. */
    bool four_screen;   /* Provide four-screen VRAM. */
    bool tv_system;     /* TV system (0: NTSC; 1: PAL). */
} Cartridge;

bool cartridge_load(Cartridge *cartridge, byte *data, int length);

#endif
