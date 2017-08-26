#include "../include/ppu.h"
#include "../include/vram.h"

static byte vram[0x2000];

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

inline void vrm_init(void) {
    for (int i = 0; i < 0x2000; i++) {
        vram[i] = 0x00;
    }
}

inline void vrm_set_mode(MirrorMode mode_) {
    mode = mode_;
}

inline byte vrm_read(word address) {
    address &= 0x3FFF;

    /* 0x0000 - 0x1FFF: Pattern tables. */
    if (address < 0x2000) {
        return vram[address];
    }

    /* 0x2000 - 0x3EFF: Nametables. */
    else if (address < 0x3F00) {
        if (mode == MMC) {
            return 0x00; /* TODO */
        }
        else {
            address = mirror(address);
            return ppu_nametable_read(address);
        }
    }

    /* 0x3F00 - 0x3FFF: Palettes. */
    return ppu_palette_read(address & 0x1F);
}

inline void vrm_write(word address, byte data) {
    address &= 0x3FFF;

    /* 0x0000 - 0x1FFF: Pattern tables. */
    if (address < 0x2000) {
        vram[address] = data;
    }

    /* 0x2000 - 0x3EFF: Nametables. */
    else if (address < 0x3F00) {
        if (mode == MMC) {
            /* ... */
        }
        else {
            address = mirror(address);
            ppu_nametable_write(address, data);
        }
    }

    /* 0x3F00 - 0x3FFF: Palettes. */
    else {
        ppu_palette_write(address & 0x1F, data);
    }
}
