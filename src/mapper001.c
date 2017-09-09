/* -----------------------------------------------------------------
 * MMC1 (mapper 1).
 * -------------------------------------------------------------- */

#include <stdlib.h>
#include "../include/log.h"
#include "../include/mapper001.h"
#include "../include/vram.h"

#define NUM_REGISTERS 5

#define shift_register  cartridge->registers[0]
#define control         cartridge->registers[1]
#define chr_bank_0      cartridge->registers[2]
#define chr_bank_1      cartridge->registers[3]
#define prg_bank        cartridge->registers[4]

static inline void write_register(Cartridge *cartridge, word address) {
    if (address < 0xA000) {
        /* Set mirroring mode. */
        switch (shift_register & 0x3) {
            case 0: vrm_set_mode(SINGLE_0);   break;
            case 1: vrm_set_mode(SINGLE_1);   break;
            case 2: vrm_set_mode(VERTICAL);   break;
            case 3: vrm_set_mode(HORIZONTAL); break;
        }
    }
    else if (address < 0xC000) {
        chr_bank_0 = shift_register;
    }
    else if (address < 0xE000) {
        chr_bank_1 = shift_register;
    }
    else {
        prg_bank = shift_register;
    }
}

static inline void load_register(Cartridge *cartridge, word address, byte data) {
    if (data & 0x80) {
        shift_register = 0x10;
    }
    else {
        bool filled = shift_register & 0x01;
        shift_register >>= 1;
        shift_register |= ((data & 0x01) << 4);
        if (filled) {
            write_register(cartridge, address);
            shift_register = 0x10;
        }
    }
}

static inline byte mapper001_cpu_read(Cartridge *cartridge, word address) {
    if (address >= 0x6000) {
        /* CPU 0x6000-0x7FFF: 8KB PRG RAM bank (fixed). */
        if (address < 0x8000) {
            return cartridge->prg_ram[address - 0x6000];
        }
        else {
            /* ... */
        }
    }
    else {
        LOG_ERROR("Invalid address %04X at CPU read (mapper 1).", address);
    }
}

static inline void mapper001_cpu_write(Cartridge *cartridge, word address, byte data) {
    if (address >= 0x6000) {
        /* CPU 0x6000-0x7FFF: 8KB PRG RAM bank (fixed). */
        if (address < 0x8000) {
            cartridge->prg_ram[address - 0x6000] = data;
        }
        else {
            load_register(cartridge, address, data);
        }
    }
    else {
        LOG_ERROR("Invalid address $%04X at CPU write (mapper 1).", address);
    }
}

static inline byte mapper001_ppu_read(Cartridge *cartridge, word address) {
    if (address < 0x2000) {
        /* PPU 0x0000-0x0FFF: 4KB switchable CHR bank. */
        if (address < 0x1000) {
            /* ... */
        }
        /* PPU 0x1000-0x1FFF: 4KB switchable CHR bank. */
        else {
            /* ... */
        }
    }
    else {
        LOG_ERROR("Invalid address $%04X at PPU read (mapper 1).", address);
    }
}

static inline void mapper001_ppu_write(Cartridge *cartridge, word address, byte data) {
    if (address < 0x2000) {
        /* PPU 0x0000-0x0FFF: 4KB switchable CHR bank. */
        if (address < 0x1000) {
            /* ... */
        }
        /* PPU 0x1000-0x1FFF: 4KB switchable CHR bank. */
        else {
            /* ... */
        }
    }
    else {
        LOG_ERROR("Invalid address $%04X at PPU write (mapper 1).", address);
    }
}

void mapper001_init(Cartridge *cartridge) {
    /* Allocate 8KB of PRG RAM. */
    cartridge->prg_ram = malloc(0x1000);
    for (int i = 0; i < 0x1000; i++) {
        cartridge->prg_ram[i] = 0x00;
    }

    /* Initialize registers. */
    cartridge->registers = malloc(NUM_REGISTERS);
    shift_register = 0x10;

    /* Initialize mapper functions. */
    cartridge->cpu_read  = mapper001_cpu_read;
    cartridge->cpu_write = mapper001_cpu_write;
    cartridge->ppu_read  = mapper001_ppu_read;
    cartridge->ppu_write = mapper001_ppu_write;
}
