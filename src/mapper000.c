/* -----------------------------------------------------------------
 * NROM (mapper 0).
 * -------------------------------------------------------------- */

#include <stdlib.h>
#include "../include/log.h"
#include "../include/mapper000.h"

static byte mapper000_cpu_read(Cartridge *cartridge, word address) {
    if (address >= 0x6000) {
        /* CPU 0x6000-0x7FFF: 8 KB PRG RAM. */
        if (address < 0x8000) {
            return cartridge->prg_ram[address - 0x6000];
        }
        /* CPU 0x8000-0xBFFF: First 16 KB of ROM. */
        else if (address < 0xC000) {
            return cartridge->prg_rom[address - 0x8000];
        }
        /* CPU 0xC000-0xFFFF: Last 16 KB of ROM (NROM-256) or mirror (NROM-128). */
        else {
            word offset = cartridge->prg_banks > 1 ? 0x8000 : 0xC000;
            return cartridge->prg_rom[address - offset];
        }
    }
    else {
        LOG_ERROR("Invalid address %04X at CPU read (mapper 0).", address);
    }
}

static void mapper000_cpu_write(Cartridge *cartridge, word address, byte data) {
    /* CPU 0x6000-0x7FFF: 8KB PRG RAM. */
    if (address >= 0x6000 && address < 0x8000) {
        cartridge->prg_ram[address - 0x6000] = data;
    }
    else {
        LOG_ERROR("Invalid address %04X at CPU write (mapper 0).", address);
    }
}

static byte mapper000_ppu_read(Cartridge *cartridge, word address) {
    /* PPU 0x0000-0x1FFF: 8KB CHR ROM. */
    if (address < 0x2000) {
        return cartridge->chr_rom[address];
    }
    else {
        LOG_ERROR("Invalid address $%04X at PPU read (mapper 0).", address);
    }
}

static void mapper000_ppu_write(Cartridge *cartridge, word address, byte data) {
    /* PPU 0x0000-0x1FFF: 8KB CHR ROM. */
    if (address < 0x2000) {
        cartridge->chr_rom[address] = data;
    }
    else {
        LOG_ERROR("Invalid address $%04X at PPU write (mapper 0).", address);
    }
}

void mapper000_init(Cartridge *cartridge) {
    /* Allocate 8KB of PRG RAM. */
    cartridge->prg_ram = malloc(0x2000);
    for (int i = 0; i < 0x2000; i++) {
        cartridge->prg_ram[i] = 0x00;
    }

    /* Initialize mapper. */
    cartridge->cpu_read  = mapper000_cpu_read;
    cartridge->cpu_write = mapper000_cpu_write;
    cartridge->ppu_read  = mapper000_ppu_read;
    cartridge->ppu_write = mapper000_ppu_write;
}
