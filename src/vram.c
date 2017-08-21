#include "../include/vram.h"

static byte vram[VRAM_SIZE];

inline void vrm_init(void) {
    for (int i = 0; i < VRAM_SIZE; i++) {
        vram[i] = 0x00;
    }
}

inline byte vrm_read(word address) {
    address = address % 0x4000;
    if (address >= 0x3F20) {
        address = 0x3F00 + (address % 0x20);
    }
    return vram[address];
}

inline void vrm_write(word address, byte data) {
    address = address % 0x4000;

    if (address < 0x2000) {
        vram[address] = data;
    }
    else if (address < 0x2F00) {
        vram[address] = data;
        vram[address + 0x1000] = data;
    }
    else if (address < 0x3000) {
        vram[address] = data;
    }
    else if (address < 0x3F00) {
        vram[address] = data;
        vram[address - 0x1000] = data;
    }
    else {
        address = 0x3F00 + (address % 0x20);

        /* Background color. */
        if (address % 0x04 == 0) {
            vram[0x3F00]  = data;
            vram[0x3F04]  = data;
            vram[0x3F08]  = data;
            vram[0x3F0C]  = data;
            vram[0x3F10]  = data;
            vram[0x3F14]  = data;
            vram[0x3F18]  = data;
            vram[0x3F1C]  = data;
        } else {
            vram[address] = data;
        }
    }
}
