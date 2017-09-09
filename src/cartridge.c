#include <stdio.h>
#include <stdlib.h>
#include "../include/cartridge.h"
#include "../include/log.h"

/* iNES header ("NES" followed by MS-DOS end-of-file). */
static const byte NES_HEADER[4] = {0x4E, 0x45, 0x53, 0x1A};

bool cartridge_load(Cartridge *cartridge, byte *data, int length) {
    /* Check if there is enough data to read the header. */
    if (length < INES_HEADER_SIZE) {
        LOG_ERROR("Insufficient data to load rom.");
    }

    /* Check for .NES file. */
    for (int i = 0; i < 4; i++) {
        if (data[i] != NES_HEADER[i]) {
            LOG_ERROR("Incorrect rom header.");
        }
    }

    /* Check for NES 2.0 format. */
    if (data[7] & 0x0C == 0x08) {
        LOG_ERROR("NES 2.0 format not supported.");
    }

    /* Read header information. */
    cartridge->prg_banks    = data[4];
    cartridge->chr_banks    = data[5];
    cartridge->mirroring    = data[6] & 0x01;
    cartridge->sram         = data[6] & 0x02;
    cartridge->four_screen  = data[6] & 0x08;
    cartridge->tv_system    = data[9] & 0x01;
    cartridge->mapper       = (data[7] & 0xF0) | ((data[6] & 0xF0) >> 4);

    cartridge->prg_rom      = NULL;
    cartridge->prg_ram      = NULL;
    cartridge->chr_rom      = NULL;
    cartridge->chr_ram      = NULL;
    cartridge->registers    = NULL;
    cartridge->cpu_read     = NULL;
    cartridge->cpu_write    = NULL;
    cartridge->ppu_read     = NULL;
    cartridge->ppu_write    = NULL;

    int prg_size_in_bytes   = cartridge->prg_banks * PRG_BANK_SIZE;
    int chr_size_in_bytes   = cartridge->chr_banks * CHR_BANK_SIZE;

    /* Check if there is enough data. */
    if (length < INES_HEADER_SIZE + (data[6] & 0x04) * INES_TRAINER_SIZE +
            prg_size_in_bytes + chr_size_in_bytes) {
        LOG_ERROR("Insufficient data to load rom.");
    }

    /* Start reading at byte 16; ignore 512 byte trainer if present. */
    int offset = data[6] & 0x04 ? INES_HEADER_SIZE + INES_TRAINER_SIZE :
        INES_HEADER_SIZE;

    /* Read PRG ROM data. */
    if (prg_size_in_bytes > 0) {
        cartridge->prg_rom = malloc(prg_size_in_bytes);
        if (cartridge->prg_rom) {
            for (int i = 0; i < prg_size_in_bytes; i++) {
                cartridge->prg_rom[i] = data[i + offset];
            }
            offset += prg_size_in_bytes;
        }
        else {
            LOG_ERROR("Unable to allocate memory for PRG ROM.");
        }
    }
    else {
        LOG_ERROR("No PRG ROM data found.");
    }
    
    /* Read CHR ROM data. */
    if (chr_size_in_bytes > 0) {
        cartridge->chr_rom = malloc(chr_size_in_bytes);
        if (cartridge->chr_rom) {
            for (int i = 0; i < chr_size_in_bytes; i++) {
                cartridge->chr_rom[i] = data[i + offset];
            }
        }
        else {
            LOG_ERROR("Unable to allocate memory for CHR ROM.");
        }
    }

    /* Ignore the rest. */

    return true;
}
