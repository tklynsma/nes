#include "../include/cpu_flags.h"
#include "../include/cpu_internal.h"

static byte lo, hi;

inline word cpu_fetch_16(void) {
    lo = cpu_fetch();
    hi = cpu_fetch();
    return (hi << 8) | lo;
}

inline word cpu_read_16(word address) {
    lo = cpu_read(address);
    hi = cpu_read(address + 1);
    return (hi << 8) | lo;
}

inline void cpu_push(byte data) {
    cpu.ram[0x100 | cpu.S--] = data;
    cycles++;
}

inline void cpu_push_address(word address) {
    cpu.ram[0x100 | cpu.S--] = address >> 8;
    cpu.ram[0x100 | cpu.S--] = address & 0xFF;
    cycles += 2;
}

inline byte cpu_pop(void) {
    cycles++;
    return cpu.ram[0x100 | ++cpu.S];
}

inline word cpu_pop_address(void) {
    cycles += 2;
    lo = cpu.ram[0x100 | ++cpu.S];
    hi = cpu.ram[0x100 | ++cpu.S];
    return (hi << 8) | lo;
}

inline void cpu_interrupt(word vector) {
    cycles += 2;
    cpu_push_address(cpu.PC);
    cpu_push(flg_get_status(false));
    cpu.PC = cpu_read_16(vector);
    flg_set_I();
}
