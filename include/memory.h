#ifndef MEMORY_H
#define MEMORY_H

#include "common.h"

#define MEM_SIZE 65536

void mem_init(void);
byte mem_read_byte(word address);
word mem_read_word(word address);
void mem_write(word address, byte data);

#endif /* MEMORY_H */
