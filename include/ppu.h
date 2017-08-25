#ifndef PPU_H
#define PPU_H

#define OAM_SIZE 256

#include "common.h"

byte ppu_io_read (word address);
void ppu_io_write(word address, byte data);

#endif /* PPU_H */
