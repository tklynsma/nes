/* -----------------------------------------------------------------
 * MMC1 (mapper 1).
 * -------------------------------------------------------------- */

#include <stdlib.h>
#include "../include/log.h"
#include "../include/mapper001.h"
#include "../include/vram.h"

#define NUM_REGISTERS 10

#define shift_register  cartridge->registers[0]
#define prg_bank_mode   cartridge->registers[1]
#define prg_bank        cartridge->registers[2]
#define chr_bank_mode   cartridge->registers[3]
#define chr_bank_0      cartridge->registers[4]
#define chr_bank_1      cartridge->registers[5]

#define prg_page_0      cartridge->registers[6]
#define prg_page_1      cartridge->registers[7]
#define chr_page_0      cartridge->registers[8]
#define chr_page_1      cartridge->registers[9]

/* -----------------------------------------------------------------
 * MMC1 register control.
 * -------------------------------------------------------------- */

static void update_banks(Cartridge *cartridge) {
    /* 0, 1: Switch 32 KB at 0x8000, ignoring low bit of bank number.  *
     * 2:    Fix first bank at 0x8000 and switch 16 KB bank at 0xC000. *
     * 3:    Fix last bank at 0xC000 and switch 16 KB bank at 0x8000.  */
    switch (prg_bank_mode) {
        case 0:
        case 1:
            prg_page_0 = prg_bank & 0x0E;
            prg_page_1 = (prg_bank & 0x0E) + 1;
            break;
        case 2:
            prg_page_0 = 0;
            prg_page_1 = prg_bank;
            break;
        case 3:
            prg_page_0 = prg_bank;
            prg_page_1 = cartridge->prg_banks - 1;
            break;
    }

    /* 0: Switch 8 KB at a time; 1: Switch two separate 4 KB banks */
    switch (chr_bank_mode) {
        case 0:
            chr_page_0 = chr_bank_0 & 0x1E;
            chr_page_1 = (chr_bank_0 & 0x1E) + 1;
            break;
        case 1:
            chr_page_0 = chr_bank_0;
            chr_page_1 = chr_bank_1;
            break;
    }
}

static inline void write_control(Cartridge *cartridge) {
    /* Set mirroring mode. */
    switch (shift_register & 0x03) {
        case 0: vrm_set_mode(SINGLE_0);   break;
        case 1: vrm_set_mode(SINGLE_1);   break;
        case 2: vrm_set_mode(VERTICAL);   break;
        case 3: vrm_set_mode(HORIZONTAL); break;
    }

    /* PRG and CHR ROM bank mode. */
    prg_bank_mode = (shift_register & 0x0C) >> 2;
    chr_bank_mode = (shift_register & 0x10) >> 4;

    update_banks(cartridge);
}

static inline void write_register(Cartridge *cartridge, word address) {
    /* 0x8000-0x9FFF: Control register. */
    if (address < 0xA000) {
        write_control(cartridge);
    }
    /* 0xA000-0xBFFF: CHR bank 0. */
    else if (address < 0xC000) {
        chr_bank_0 = shift_register;
    }
    /* 0xC000-0xDFFF: CHR bank 1. */
    else if (address < 0xE000) {
        chr_bank_1 = shift_register;
    }
    /* 0xE000-0xFFFF: PRG bank. */
    else {
        prg_bank = shift_register & 0x0F;
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
            update_banks(cartridge);
            shift_register = 0x10;
        }
    }
}

/* -----------------------------------------------------------------
 * MMC1 read/write.
 * -------------------------------------------------------------- */

static inline byte mapper001_cpu_read(Cartridge *cartridge, word address) {
    if (address >= 0x6000) {
        /* CPU 0x6000-0x7FFF: 8KB PRG RAM bank (fixed). */
        if (address < 0x8000) {
            return cartridge->prg_ram[address - 0x6000];
        }
        /* CPU 0x8000-0xBFFF: 16 KB PRG ROM bank (switchable). */
        else if (address < 0xC000) {
            address -= 0x8000;
            return cartridge->prg_rom[prg_page_0 * 0x4000 + address];
        }
        /* CPU 0xC000-0xFFFF: 16 KB PRG ROM bank (switchable). */
        else {
            address -= 0xC000;
            return cartridge->prg_rom[prg_page_1 * 0x4000 + address];
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
        /* CPU 0x8000-0xFFFF: Load register. */
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
        /* PPU 0x0000-0x0FFF: 4 KB CHR bank (switchable). */
        if (address < 0x1000) {
            return cartridge->chr_rom[(chr_page_0 << 12) | address];
        }
        /* PPU 0x1000-0x1FFF: 4KB switchable CHR bank. */
        else {
            address &= 0x0FFF;
            return cartridge->chr_rom[(chr_page_1 << 12) | address];
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
            cartridge->chr_rom[(chr_page_0 << 12) | address] = data;
        }
        /* PPU 0x1000-0x1FFF: 4KB switchable CHR bank. */
        else {
            address &= 0x0FFF;
            cartridge->chr_rom[(chr_page_1 << 12) | address] = data;
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
    for (int i = 0; i < NUM_REGISTERS; i++) {
        cartridge->registers[i] = 0x00;
    }
    shift_register = 0x10;
    prg_page_1 = cartridge->prg_banks - 1;

    /* Initialize mapper functions. */
    cartridge->cpu_read  = mapper001_cpu_read;
    cartridge->cpu_write = mapper001_cpu_write;
    cartridge->ppu_read  = mapper001_ppu_read;
    cartridge->ppu_write = mapper001_ppu_write;
}
