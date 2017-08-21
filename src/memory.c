#include "../include/memory.h"

static byte memory[MEM_SIZE];

inline void mem_init(void) {
    for (int i = 0; i < MEM_SIZE; i++) {
        memory[i] = 0x00;
    }
}

inline byte mem_read_8(word address) {
    if (address >= 0x2000 && address < 0x4000) {
        return memory[0x2000 + (address % 0x08)];
    }
    return memory[address];
}

inline word mem_read_16(word address) {
    return (mem_read_8(address + 1) << 8) | mem_read_8(address);
}

inline void mem_write(word address, byte data) {
    /* 0x0000 - 0x2000 mirrors 0x0000 - 0x07FF. */
    if (address < 0x2000) {
        memory[ address % 0x800]           = data;
        memory[(address % 0x800) + 0x0800] = data;
        memory[(address % 0x800) + 0x1000] = data;
        memory[(address % 0x800) + 0x1800] = data;
    }

    /* 0x2000 - 0x4000 mirrors 0x2000 - 0x2008. */
    else if (address < 0x4000) {
        memory[0x2000 + (address % 0x08)] = data;
    }

    else {
        memory[address] = data;
    }
}
