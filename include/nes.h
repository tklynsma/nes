#ifndef NES_H
#define NES_H

#include "../include/common.h"

void nes_init(void);
void nes_reset(void);
bool nes_insert_cartridge(byte *data, int length);

void nes_controller1_set(int keycode, bool value);
void nes_controller2_set(int keycode, bool value);
byte nes_controller1_read(void);
byte nes_controller2_read(void);
byte nes_controller1_get(void);
byte nes_controller2_get(void);
void nes_controller1_write(byte data);
void nes_controller2_write(byte data);

#endif /* NES_H */
