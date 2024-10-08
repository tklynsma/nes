#ifndef CPU_H
#define CPU_H

#define NMI_VECTOR   0xFFFA
#define RESET_VECTOR 0xFFFC
#define IRQ_VECTOR   0xFFFE

#include "../include/common.h"

void cpu_set_nmi(void);

void cpu_init(void);                    /* Initialize the CPU. */
void cpu_reset(void);                   /* Reset the CPU status. */
void cpu_execute(void);                 /* Execute the next CPU instruction. */
void cpu_suspend(int num_cycles);       /* Suspend the cpu for some number of cycles. */     
unsigned long long cpu_get_ticks(void); /* Get the total number of cycles run. */

byte cpu_ram_read (word address);
void cpu_ram_write(word address, byte data);

#endif /* CPU_H */
