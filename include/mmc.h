#ifndef MMC_H
#define MMC_H

#include "cartridge.h"
#include "common.h"
#include "nes.h"

void mmc_init(Cartridge *cartridge_);

byte mmc_cpu_get  (word address);
byte mmc_cpu_read (word address);
void mmc_cpu_write(word address, byte data);

byte mmc_ppu_read (word address);
void mmc_ppu_write(word address, byte data);

#endif /* MMC_H */
