#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "common.h"

#define NUM_BUTTONS     8

#define BUTTON_A        0
#define BUTTON_B        1
#define BUTTON_SELECT   2
#define BUTTON_START    3
#define BUTTON_UP       4
#define BUTTON_DOWN     5
#define BUTTON_LEFT     6
#define BUTTON_RIGHT    7

typedef struct {
    bool strobe;                /* Controller shift register strobe. */
    bool button[NUM_BUTTONS];   /* Button status. */
    byte index;                 /* Index of the next button to be read back. */
} Controller;

void controller_init(Controller *controller);
byte controller_get (Controller *controller);
byte controller_read(Controller *controller);
void controller_write(Controller *controller, byte data);
void controller_set_key(Controller *controller, int keycode, bool value);

#endif /* CONTROLLER_H */
