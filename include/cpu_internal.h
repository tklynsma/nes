#ifndef CPU_INTERNAL_H
#define CPU_INTERNAL_H

#include "common.h"
#include "memory.h"

#define RAM_SIZE 0x800

/* Micro operations. */

#define cpu_fetch() mem_read(cpu.PC++); cycles++;
#define cpu_fetch_dummy() mem_read(cpu.PC); cycles++;
#define cpu_read(a) mem_read(a); cycles++;
#define cpu_write(a, d) mem_write(a, d); cycles++;

word cpu_fetch_16(void);
word cpu_read_16(word address);
void cpu_push(byte data);
void cpu_push_address(word address);
byte cpu_pop(void);
word cpu_pop_address(void);

/* Handle interrupts. */

void cpu_interrupt(word vector);

/* CPU status. */

typedef struct {
    word PC;             /* Program counter. */
    byte S;              /* Stack pointer. */
    byte A;              /* Accumulator. */
    byte X, Y;           /* Index registers. */
    byte ram[RAM_SIZE];  /* The CPU's RAM. */
} CPU;

extern CPU cpu;
extern unsigned long long cycles;

#endif /* CPU_INTERNAL_H */
