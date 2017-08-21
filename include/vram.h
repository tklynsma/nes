#ifndef VRAM_H
#define VRAM_H

#include "common.h"

#define VRAM_SIZE 0x3F20

void vrm_init(void);
byte vrm_read(word address);
void vrm_write(word address, byte data);

#endif /* VRAM_H */
