#include "../include/cpu.h"
#include "../include/memory.h"
#include "../include/mmc.h"
#include "../include/nes.h"
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
    else if (address == 0x4016) {
        return nes_controller1_read();
    }
    else if (address == 0x4017) {
        return nes_controller2_read();
    }

    /* 0x4020 - 0xFFFF: Cartridge space. */
    else if (address >= 0x4020) {
        return mmc_cpu_read(address);
    }
}

inline word mem_read_16(word address) {
    return (mem_read(address + 1) << 8) | mem_read(address);
}

/* Memory access without side effects. */
inline byte mem_get(word address) {
    /* 0x0000 - 0x1FFF: RAM. */
    if (address < 0x2000) {
        return cpu_ram_read(address & 0x7FF);
    }

    /* 0x2000 - 0x401F: I/O registers. */
    else if (address < 0x4000) {
        return ppu_io_get(address);
    }
    else if (address == 0x4014) {
        return ppu_dma_read();
    }
    else if (address == 0x4016) {
        return nes_controller1_get();
    }
    else if (address == 0x4017) {
        return nes_controller2_get();
    }

    /* 0x4020 - 0xFFFF: Cartridge space. */
    else if (address >= 0x4020) {
        return mmc_cpu_get(address);
    }
}

inline word mem_get_16(word address) {
    return (mem_get(address + 1) << 8) | mem_get(address);
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
    else if (address == 0x4016) {
        nes_controller1_write(data);
    }
    else if (address == 0x4017) {
        nes_controller2_write(data);
    }

    /* 0x4020 - 0xFFFF: Cartridge space. */
    else if (address >= 0x4020) {
        mmc_cpu_write(address, data);
    }
}
