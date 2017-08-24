#ifndef CPU_H
#define CPU_H

#define RAM_SIZE 0x800

#include "../include/common.h"

void cpu_init(void);                /* Initialize the CPU. */
void cpu_reset(void);               /* Reset the CPU status. */
void cpu_cycle(int num_cycles);     /* Execute the next CPU cycle(s). */

byte cpu_ram_read (word address);               /* Read a byte from RAM. */
void cpu_ram_write(word address, byte data);    /* Write a byte to RAM. */

#endif /* CPU_H */
