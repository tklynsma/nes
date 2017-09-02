#include "../include/cpu.h"
#include "../include/memory.h"
#include "../include/mmc.h"
#include "../include/ppu.h"

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

    /* 0x4020 - 0xFFFF: Cartridge space. */
    else if (address >= 0x4020) {
        return mmc_cpu_read(address);
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

    /* 0x4020 - 0xFFFF: Cartridge space. */
    else if (address >= 0x4020) {
        mmc_cpu_write(address, data);
    }
}
