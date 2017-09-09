#ifndef CPU_H
#define CPU_H

#define RAM_SIZE 0x800

#define NMI_VECTOR   0xFFFA
#define RESET_VECTOR 0xFFFC
#define IRQ_VECTOR   0xFFFE

#define CPU_LOGGING

#ifdef CPU_LOGGING
# define LOG_CPU(...) log_cpu(__VA_ARGS__)
#else
# define LOG_CPU(...) do {} while (0)
#endif

#include "../include/common.h"

void cpu_set_nmi(void);

void cpu_init(void);                    /* Initialize the CPU. */
void cpu_reset(void);                   /* Reset the CPU status. */
void cpu_cycle(int num_cycles);         /* Execute the next CPU cycle(s). */
void cpu_suspend(int num_cycles);       /* Suspend the cpu for some number of cycles. */     
int cpu_get_wait_ticks(void);           /* Get the number of cycles until the next instruction. */
unsigned long long cpu_get_ticks(void); /* Get the total number of cycles run. */

byte cpu_ram_read (word address);
void cpu_ram_write(word address, byte data);

#endif /* CPU_H */
