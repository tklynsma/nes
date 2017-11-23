#include "../include/cpu_flags.h"
#include "../include/cpu_internal.h"

static byte lo, hi;

inline word fetch_16(void) {
    lo = fetch();
    hi = fetch();
    return (hi << 8) | lo;
}

inline word read_16(word address) {
    lo = read(address);
    hi = read(address + 1);
    return (hi << 8) | lo;
}

inline void push(byte data) {
    cpu.ram[0x100 | cpu.S--] = data;
    cycles++;
}

inline void push_address(word address) {
    cpu.ram[0x100 | cpu.S--] = address >> 8;
    cpu.ram[0x100 | cpu.S--] = address & 0xFF;
    cycles += 2;
}

inline byte pop(void) {
    cycles++;
    return cpu.ram[0x100 | ++cpu.S];
}

inline word pop_address(void) {
    cycles += 2;
    lo = cpu.ram[0x100 | ++cpu.S];
    hi = cpu.ram[0x100 | ++cpu.S];
    return (hi << 8) | lo;
}

inline void interrupt(word vector) {
    cycles += 2;
    push_address(cpu.PC);
    push(flg_get_status(false));
    cpu.PC = read_16(vector);
    flg_set_I();
}
