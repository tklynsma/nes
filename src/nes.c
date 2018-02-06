#include <stdio.h>
#include <stdlib.h>
#include "../include/cartridge.h"
#include "../include/controller.h"
#include "../include/mmc.h"
#include "../include/nes.h"

static Cartridge cartridge;
static Controller controller1;
static Controller controller2;

void nes_init(void) {
    controller_init(&controller1);
    controller_init(&controller2);
}

bool nes_insert_cartridge(byte *data, int length) {
    bool success = cartridge_load(&cartridge, data, length);
    mmc_init(&cartridge);
    return success;
}

inline void nes_controller1_set(int keycode, bool value) {
    controller_set_key(&controller1, keycode, value);
}

inline void nes_controller2_set(int keycode, bool value) {
    controller_set_key(&controller2, keycode, value);
}

inline byte nes_controller1_read(void) {
    return controller_read(&controller1);
}

inline byte nes_controller2_read(void) {
    return controller_read(&controller2);
}

inline byte nes_controller1_get(void) {
    return controller_get(&controller1);
}

inline byte nes_controller2_get(void) {
    return controller_get(&controller2);
}

inline void nes_controller1_write(byte data) {
    controller_write(&controller1, data);
}

inline void nes_controller2_write(byte data) {
    controller_write(&controller2, data);
}
