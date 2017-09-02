#ifndef CPU_INTERNAL_H
#define CPU_INTERNAL_H

#include "common.h"

typedef struct {
    word PC;             /* Program counter. */
    byte S;              /* Stack pointer. */
    byte A;              /* Accumulator. */
    byte X, Y;           /* Index registers. */
    byte ram[RAM_SIZE];  /* The CPU's RAM. */
} CPU;

extern CPU cpu;          /* The CPU status. */

#endif /* CPU_INTERNAL_H */
