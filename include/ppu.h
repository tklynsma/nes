#ifndef PPU_H
#define PPU_H

#include "common.h"

void ppu_init(void);
void ppu_reset(void);

byte ppu_io_read(word address);
void ppu_io_write(word address, byte data);

byte ppu_dma_read();
void ppu_dma_write(byte data);

byte ppu_palette_read(word address);
void ppu_palette_write(word address, byte data);

byte ppu_nametable_read(word address);
void ppu_nametable_write(word address, byte data);

void ppu_render_frame(byte display[256][240]);

#endif /* PPU_H */
