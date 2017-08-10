#ifndef CPU_H
#define CPU_H

void cpu_init(void);                /* Initialize the CPU. */
void cpu_reset(void);               /* Reset the CPU status. */
void cpu_cycle(int num_cycles);     /* Execute the next CPU cycle(s). */

#endif /* CPU_H */
