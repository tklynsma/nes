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
    if (address < 0x2000) {             /* 0x0000 - 0x1FFF */
        return cpu_ram_read(address);
    }
    else if (address < 0x4000) {        /* 0x2000 - 0x3FFF */
        return ppu_io_read(address);
    }
    else if (address == 0x4014) {       /* 0x4014: OAM DMA */
        return ppu_dma_read();
    }
    else {                              /* TODO */
        return memory[address];
    }
}

inline word mem_read_16(word address) {
    return (mem_read(address + 1) << 8) | mem_read(address);
}

inline void mem_write(word address, byte data) {
    if (address < 0x2000) {             /* 0x0000 - 0x1FFF */
        cpu_ram_write(address, data);
    }
    else if (address < 0x4000) {        /* 0x2000 - 0x3FFF */
        ppu_io_write(address, data);
    }
    else if (address == 0x4014) {       /* 0x4014: OAM DMA */
        ppu_dma_write(data);
    }
    else {                              /* TODO */
        memory[address] = data;
    }
}
