#ifndef CPU_INTERNAL_H
#define CPU_INTERNAL_H

#include "common.h"
#include "memory.h"

#define RAM_SIZE 0x800

/* Micro operations. */

#define fetch() mem_read(cpu.PC++); cycles++;
#define fetch_dummy() mem_read(cpu.PC); cycles++;
#define read(a) mem_read(a); cycles++;
#define write(a, d) mem_write(a, d); cycles++;

word fetch_16(void);
word read_16(word address);
void push(byte data);
void push_address(word address);
byte pop(void);
word pop_address(void);

/* Handle interrupts. */

void interrupt(word vector);

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
