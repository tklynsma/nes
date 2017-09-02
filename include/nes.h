#ifndef NES_H
#define NES_H

#include <stdio.h>
#include "../include/common.h"

void nes_init(void);
void nes_reset(void);
bool nes_insert_cartridge(byte *data, int length);

#endif /* NES_H */
