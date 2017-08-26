#ifndef VRAM_H
#define VRAM_H

#include "common.h"

typedef enum { HORIZONTAL = 0, VERTICAL, SINGLE_0, SINGLE_1, MMC } MirrorMode;

void vrm_init(void);
void vrm_set_mode(MirrorMode mode_);
byte vrm_read(word address);
void vrm_write(word address, byte data);

#endif /* VRAM_H */
