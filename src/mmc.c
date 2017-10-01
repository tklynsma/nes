#include <stdlib.h>
#include "../include/cartridge.h"
#include "../include/log.h"
#include "../include/mapper000.h"
#include "../include/mapper001.h"
#include "../include/mmc.h"
#include "../include/nes.h"
#include "../include/vram.h"

static Cartridge *cartridge = NULL;

void mmc_init(Cartridge *cartridge_) {
    if (cartridge != NULL) {
        LOG_WARNING("Cartridge already initialized.");
    }

    if (cartridge_ == NULL) {
        LOG_ERROR("Cartridge not allocated.");
    }

    cartridge = cartridge_;

    switch (cartridge->mapper) {
        case 0:  mapper000_init(cartridge); break;
        case 1:  mapper001_init(cartridge); break;

        default: LOG_ERROR("Unsupported mapper: %d.", cartridge->mapper);
    }

    MirrorMode mode = cartridge->four_screen ? MMC :
        cartridge->mirroring ? VERTICAL : HORIZONTAL;
    vrm_set_mode(mode);
}

inline byte mmc_cpu_read(word address) {
    return (*cartridge->cpu_read)(cartridge, address);
}

inline byte mmc_cpu_get(word address) {
    return (*cartridge->cpu_get)(cartridge, address);
}

inline void mmc_cpu_write(word address, byte data) {
    if (cartridge->cpu_write != NULL) {
        (*cartridge->cpu_write)(cartridge, address, data);
    }
}

inline byte mmc_ppu_read(word address) {
    return (*cartridge->ppu_read)(cartridge, address);
}

inline void mmc_ppu_write(word address, byte data) {
    if (cartridge->ppu_write != NULL) {
        (*cartridge->ppu_write)(cartridge, address, data);
    }
}
