#include <stdio.h>
#include <stdlib.h>

#include "../include/mmc.h"
#include "../include/ppu.h"
#include "../include/vram.h"

static MirrorMode mode;
static int mirror_lookup_table[4][4] = {
    {0x000, 0x000, 0x400, 0x400},
    {0x000, 0x400, 0x000, 0x400},
    {0x000, 0x000, 0x000, 0x000},
    {0x400, 0x400, 0x400, 0x400},
};

static inline word mirror(word address) {
    address &= 0xFFF;
    return mirror_lookup_table[mode][address >> 10] + (address & 0x3FF);
}

inline void vrm_set_mode(MirrorMode mode_) {
    mode = mode_;
}

inline byte vrm_read(word address) {
    address &= 0x3FFF;

    /* 0x0000 - 0x1FFF: Pattern tables. */
    if (address < 0x2000) {
        return mmc_ppu_read(address);
    }

    /* 0x2000 - 0x3EFF: Nametables. */
    else if (address < 0x3F00) {
        if (mode == MMC) {
            return mmc_ppu_read(address);
        }
        else {
            address = mirror(address);
            return ppu_nametable_read(address);
        }
    }

    /* 0x3F00 - 0x3FFF: Palettes. */
    else {
        return ppu_palette_read(address);
    }
}

inline void vrm_write(word address, byte data) {
    address &= 0x3FFF;

    /* 0x0000 - 0x1FFF: Pattern tables. */
    if (address < 0x2000) {
        mmc_ppu_write(address, data);
    }

    /* 0x2000 - 0x3EFF: Nametables. */
    else if (address < 0x3F00) {
        if (mode == MMC) {
            mmc_ppu_write(address, data);
        }
        else {
            address = mirror(address);
            ppu_nametable_write(address, data);
        }
    }

    /* 0x3F00 - 0x3FFF: Palettes. */
    else {
        ppu_palette_write(address, data);
    }
}
