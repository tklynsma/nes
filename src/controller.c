#include <stdio.h>
#include "../include/controller.h"

void controller_init(Controller *controller) {
    controller->index  = 0;
    controller->strobe = false;

    for (int i = 0; i < NUM_BUTTONS; i++) {
        controller->button[i] = false;
    }
}

byte controller_read(Controller *controller) {
    /* While strobe is high, return the current state of the first button (A). */
    if (controller->strobe) {
        return controller->button[BUTTON_A];
    }

    /* Otherwise, the first 8 reads will indicate which buttons are pressed,
     * and all subsequent reads will return D=1. */
    else {
        if (controller->index < 8) {
            return controller->button[controller->index++] ? 0x01 : 0x00;
        }
        else {
            return 0x01;
        }
    }
}

byte controller_get(Controller *controller) {
    if (controller->strobe) {
        return controller->button[BUTTON_A];
    }
    else {
        if (controller->index < 8) {
            return controller->button[controller->index] ? 0x01 : 0x00;
        }
        else {
            return 0x01;
        }
    }
}

void controller_write(Controller *controller, byte data) {
    controller->strobe = data & 0x01;
    if (controller->strobe) {
        controller->index = 0;
    }
}

void controller_set_key(Controller *controller, int keycode, bool value) {
    if (controller->strobe) {
        controller->button[keycode] = value;
    }
    controller->button[keycode] = value;
}
