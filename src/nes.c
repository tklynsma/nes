#include <stdio.h>
#include <stdlib.h>
#include "../include/cartridge.h"
#include "../include/mmc.h"
#include "../include/nes.h"

static Cartridge cartridge;

bool nes_insert_cartridge(byte *data, int length) {
    bool success = cartridge_load(&cartridge, data, length);
    mmc_init(&cartridge);
    return success;
}
