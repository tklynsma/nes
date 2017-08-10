#include "memory.h"

byte memory[MEM_SIZE];

void mem_init(void) {
    for (int i = 0; i < MEM_SIZE; i++) {
        memory[i] = 0x00;
    }
}

inline byte mem_read_byte(word address) {
    return memory[address];
}

inline word mem_read_word(word address) {
    return (memory[address + 1] << 8) | memory[address];
}

inline void mem_write(word address, byte data) {
    memory[address] = data;
}
