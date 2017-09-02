#include <stdio.h>
#include <stdlib.h>
#include "../include/cartridge.h"

/* iNES header ("NES" followed by MS-DOS end-of-file). */
static const byte NES_HEADER[4] = {0x4E, 0x45, 0x53, 0x1A};

bool cartridge_load(Cartridge *cartridge, byte *data, int length) {
    /* Check if there is enough data to read the header. */
    if (length < INES_HEADER_SIZE) {
        printf("ERROR: Insufficient data.\n");
        return false;
    }

    /* Check for .NES file. */
    for (int i = 0; i < 4; i++) {
        if (data[i] != NES_HEADER[i]) {
            printf("ERROR: Incorrect header.\n");
            return false;
        }
    }

    /* Check for NES 2.0 format. */
    if (data[7] & 0x0C == 0x08) {
        printf("ERROR: NES 2.0 format not supported.\n");
        return false;
    }

    /* Read header information. */
    cartridge->prg_banks    = data[4];
    cartridge->chr_banks    = data[5];
    cartridge->mirroring    = data[6] & 0x01;
    cartridge->sram         = data[6] & 0x02;
    cartridge->four_screen  = data[6] & 0x08;
    cartridge->tv_system    = data[9] & 0x01;
    cartridge->mapper       = (data[7] & 0xF0) | ((data[6] & 0xF0) >> 8);

    int prg_size_in_bytes   = cartridge->prg_banks * PRG_BANK_SIZE;
    int chr_size_in_bytes   = cartridge->chr_banks * CHR_BANK_SIZE;

    /* Check if there is enough data. */
    if (length < INES_HEADER_SIZE + (data[6] & 0x04) * INES_TRAINER_SIZE +
            prg_size_in_bytes + chr_size_in_bytes) {
        printf("ERROR: Insufficient data.\n");
        return false;
    }

    /* Start reading at byte 16; ignore 512 byte trainer if present. */
    int offset = data[6] & 0x04 ? INES_HEADER_SIZE + INES_TRAINER_SIZE :
        INES_HEADER_SIZE;

    /* Read PRG ROM data. */
    cartridge->prg_rom = malloc(prg_size_in_bytes);
    if (cartridge->prg_rom) {
        for (int i = 0; i < prg_size_in_bytes; i++) {
            cartridge->prg_rom[i] = data[i + offset];
        }
        offset += prg_size_in_bytes;
    }
    else {
        printf("ERROR: Unable to allocate memory for PRG ROM.\n");
        return false;
    }
    
    /* Read CHR ROM data. */
    cartridge->chr_rom = malloc(chr_size_in_bytes);
    if (cartridge->chr_rom) {
        for (int i = 0; i < chr_size_in_bytes; i++) {
            cartridge->chr_rom[i] = data[i + offset];
        }
    }
    else {
        printf("ERROR: Unable to allocate memory for CHR ROM.\n");
        return false;
    }

    /* Ignore the rest. */

    return true;
}
