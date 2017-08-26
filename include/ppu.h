#ifndef PPU_H
#define PPU_H

#include "common.h"

byte ppu_io_read(word address);
void ppu_io_write(word address, byte data);

byte ppu_dma_read();
void ppu_dma_write(byte data);

byte ppu_palette_read(word address);
void ppu_palette_write(word address, byte data);

byte ppu_nametable_read(word address);
void ppu_nametable_write(word address, byte data);

#endif /* PPU_H */
