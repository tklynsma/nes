#include "../include/ppu.h"

typedef struct {
    byte latch;     /* PPUGenLatch. */
    byte oam[OAM_SIZE];
} PPU;

static inline byte read_ppu_status() { return 0x00; }
static inline byte read_oam_data()   { return 0x00; }
static inline byte read_ppu_data()   { return 0x00; }

static inline void write_ppu_ctrl(byte data) {}
static inline void write_ppu_mask(byte data) {}
static inline void write_oam_address(byte data) {}
static inline void write_oam_data(byte data) {}
static inline void write_ppu_scroll(byte data) {}
static inline void write_ppu_address(byte data) {}
static inline void write_ppu_data(byte data) {}
static inline void write_oam_dma(byte data) {}

static PPU ppu;

inline byte ppu_io_read(word address) {
    address = address < 0x4000 ? 0x2000 + (address % 8) : address;
    switch (address) {
        case 0x2002: return read_ppu_status();  break;
        case 0x2004: return read_oam_data();    break;
        case 0x2007: return read_ppu_data();    break;
    }
    return ppu.latch;
}

inline void ppu_io_write(word address, byte data) {
    ppu.latch = data;

    address = address < 0x4000 ? 0x2000 + (address % 8) : address;
    switch (address) {
        case 0x2000: write_ppu_ctrl(data);      break;
        case 0x2001: write_ppu_mask(data);      break;
        case 0x2003: write_oam_address(data);   break;
        case 0x2004: write_oam_data(data);      break;
        case 0x2005: write_ppu_scroll(data);    break;
        case 0x2006: write_ppu_address(data);   break;
        case 0x2007: write_ppu_data(data);      break;
        case 0x4014: write_oam_dma(data);       break;
    }
}
