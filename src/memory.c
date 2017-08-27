#include "../include/cpu.h"
#include "../include/memory.h"
#include "../include/ppu.h"

static byte memory[MEM_SIZE];

inline void mem_init(void) {
    for (int i = 0; i < MEM_SIZE; i++) {
        memory[i] = 0x00;
    }
}

inline byte mem_read(word address) {
    /* 0x0000 - 0x1FFF: RAM. */
    if (address < 0x2000) {
        return cpu_ram_read(address & 0x7FF);
    }

    /* 0x2000 - 0x401F: I/O registers. */
    else if (address < 0x4000) {
        return ppu_io_read(address);
    }
    else if (address == 0x4014) {
        return ppu_dma_read();
    }

    /* TODO: the rest. */
    else {
        return memory[address];
    }
}

inline word mem_read_16(word address) {
    return (mem_read(address + 1) << 8) | mem_read(address);
}

inline void mem_write(word address, byte data) {
    /* 0x0000 - 0x1FFF: RAM. */
    if (address < 0x2000) {
        cpu_ram_write(address & 0x7FF, data);
    }

    /* 0x2000 - 0x401F: I/O registers. */
    else if (address < 0x4000) {
        ppu_io_write(address, data);
    }
    else if (address == 0x4014) {
        ppu_dma_write(data);
    }

    /* TODO: the rest. */
    else {
        memory[address] = data;
    }
}
