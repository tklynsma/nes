#include <stdlib.h>
#include "../include/cartridge.h"
#include "../include/log.h"
#include "../include/mmc.h"
#include "../include/nes.h"
#include "../include/vram.h"

typedef void (*Init) (void);
typedef byte (*Read) (word);
typedef void (*Write)(word, byte);

typedef struct {
    Init init;
    Read read_cpu;
    Read read_ppu;
    Write write_cpu;
    Write write_ppu;
} Mapper;

static Cartridge *cartridge = NULL;
static Mapper mapper;

/* -----------------------------------------------------------------
 * Mapper 000: NROM.
 * -------------------------------------------------------------- */

static byte mapper000_read_cpu(word address) {
    if (address >= 0x8000) {
        if (address < 0xC000) {
            return cartridge->prg_rom[address - 0x8000];
        } else {
            word offset = cartridge->prg_banks > 1 ? 0x8000 : 0xC000;
            return cartridge->prg_rom[address - offset];
        }
    }
    else {
        log_error("Invalid address $%x at mapper000_read_cpu.", address);
    }
}

static byte mapper000_read_ppu(word address) {
    if (address < 0x2000) {
        return cartridge->chr_rom[address];
    }
    else {
        log_error("Invalid address $%x at mapper000_read_ppu.", address);
    }
}

/* -----------------------------------------------------------------
 * MMC.
 * -------------------------------------------------------------- */

void mmc_init(Cartridge *cartridge_) {
    if (cartridge != NULL) {
        log_warning("Cartridge already initialized.");
    }

    cartridge = cartridge_;

    mapper.init      = NULL;
    mapper.read_cpu  = mapper000_read_cpu;
    mapper.read_ppu  = mapper000_read_ppu;
    mapper.write_cpu = NULL;
    mapper.write_ppu = NULL;

    MirrorMode mode = cartridge->four_screen ? MMC :
        cartridge->mirroring ? VERTICAL : HORIZONTAL;
    vrm_set_mode(mode);
}

inline byte mmc_cpu_read(word address) {
    return (*mapper.read_cpu)(address);
}

inline void mmc_cpu_write(word address, byte data) {
    if (mapper.write_cpu != NULL) {
        (*mapper.write_cpu)(address, data);
    }
}

inline byte mmc_ppu_read(word address) {
    return (*mapper.read_ppu)(address);
}

inline void mmc_ppu_write(word address, byte data) {
    if (mapper.write_ppu != NULL) {
        (*mapper.write_ppu)(address, data);
    }
}
