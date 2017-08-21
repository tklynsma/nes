#include "../include/vram.h"

static byte vram[VRAM_SIZE / 4];

inline void vrm_init(void) {
    for (int i = 0; i < VRAM_SIZE / 4; i++) {
        vram[i] = 0x00;
    }
}

inline byte vrm_read(word address) {
    return vram[address % 0x4000];
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
        vram[address] = data;
        vram[address + 0x20] = data;
        vram[address + 0x40] = data;
        vram[address + 0x60] = data;
        vram[address + 0x80] = data;
        vram[address + 0xA0] = data;
        vram[address + 0xC0] = data;
        vram[address + 0xE0] = data;
    }
}
