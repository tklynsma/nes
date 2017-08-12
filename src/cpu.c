#include <stdio.h>  /* For testing. */
#include "common.h"
#include "cpu.h"
#include "cpu_flags.h"
#include "memory.h"

/* -----------------------------------------------------------------
 * CPU status.
 * -------------------------------------------------------------- */

typedef struct {
    word PC;    /* Program counter. */
    byte S;     /* Stack pointer. */
    byte A;     /* Accumulator. */
    byte X, Y;  /* Index registers. */
} CPU;

static CPU cpu;           /* The CPU status. */
static byte operand;      /* Operand (8 bit) of the instruction. */
static word address;      /* Operand (16 bit) of the instruction. */
static int wait_cycles;   /* The number of cycles to wait till the next operation. */
static int extra_cycles;  /* The number of additional cycles. */

/* -----------------------------------------------------------------
 * CPU addressing modes.
 * -------------------------------------------------------------- */

static inline bool is_diff_page(word address1, word address2) {
    return (address1 & 0xFF00) != (address2 & 0xFF00);
}

static inline void absolute(void) {
    address = mem_read_word(cpu.PC + 1);
    operand = mem_read_byte(address);
    cpu.PC += 3;
}

static inline void absolute_x(void) {
    address = mem_read_word(cpu.PC + 1) + cpu.X;
    operand = mem_read_byte(address);
    extra_cycles = is_diff_page(address, address - cpu.X);
    cpu.PC += 3;
}

static inline void absolute_y(void) {
    address = mem_read_word(cpu.PC + 1) + cpu.Y;
    operand = mem_read_byte(address);
    extra_cycles = is_diff_page(address, address - cpu.Y);
    cpu.PC += 3;
}

static inline void accumulator(void) {
    operand = cpu.A;
    cpu.PC += 1;
}

static inline void immediate(void) {
    operand = mem_read_byte(cpu.PC + 1);
    cpu.PC += 2;
}

static inline void implied(void) {
    cpu.PC += 1;
}

static inline void indirect(void) {
    address = mem_read_word(cpu.PC + 1);
    address = mem_read_word(address);
    cpu.PC += 3;
}

static inline void indirect_x(void) {
    address = mem_read_byte(cpu.PC + 1);
    address = mem_read_word((address + cpu.X) & 0xFF);
    operand = mem_read_byte(address);
    cpu.PC += 2;
}

static inline void indirect_y(void) {
    address = mem_read_byte(cpu.PC + 1);
    address = mem_read_word(address) + cpu.Y;
    operand = mem_read_byte(address);
    extra_cycles = is_diff_page(address, address - cpu.Y);
    cpu.PC += 2;
}

static inline void relative(void) {
    operand = mem_read_byte(cpu.PC + 1);
    cpu.PC += 2;
}

static inline void zero_page(void) {
    address = mem_read_byte(cpu.PC + 1);
    operand = mem_read_byte(address);
    cpu.PC += 2;
}

static inline void zero_page_x(void) {
    address = (mem_read_byte(cpu.PC + 1) + cpu.X) & 0xFF;
    operand = mem_read_byte(address);
    cpu.PC += 2;
}

static inline void zero_page_y(void) {
    address = (mem_read_byte(cpu.PC + 1) + cpu.Y) & 0xFF;
    operand = mem_read_byte(address);
    cpu.PC += 2;
}

/* -----------------------------------------------------------------
 * CPU instructions.
 * -------------------------------------------------------------- */

/* Branching Instructions. */
static inline void branch(bool condition) {
    if (condition) {
        address = cpu.PC;
        cpu.PC += (int8_t) operand;

        extra_cycles++;
        if (is_diff_page(address, cpu.PC)) {
            extra_cycles++;  /* Page crossed. */
        }
    }
}

static inline void bcs(void) { branch( flg_is_C()); }
static inline void bcc(void) { branch(!flg_is_C()); }
static inline void beq(void) { branch( flg_is_Z()); }
static inline void bne(void) { branch(!flg_is_Z()); }
static inline void bvs(void) { branch( flg_is_V()); }
static inline void bvc(void) { branch(!flg_is_V()); }
static inline void bmi(void) { branch( flg_is_N()); }
static inline void bpl(void) { branch(!flg_is_N()); }

/* Flag manipulation. */
static inline void clc(void) { flg_clear_C(); }
static inline void cli(void) { flg_clear_I(); }
static inline void cld(void) { flg_clear_D(); }
static inline void clv(void) { flg_clear_V(); }

static inline void sec(void) { flg_set_C(); }
static inline void sei(void) { flg_set_I(); }
static inline void sed(void) { flg_set_D(); }

/* Store & transfer instructions. */
static inline void sta(void) { mem_write(address, cpu.A); extra_cycles = 0; }
static inline void stx(void) { mem_write(address, cpu.X); }
static inline void sty(void) { mem_write(address, cpu.Y); }

static inline void txs(void) { cpu.S = cpu.X;                       }
static inline void txa(void) { cpu.A = cpu.X; flg_update_ZN(cpu.A); }
static inline void tya(void) { cpu.A = cpu.Y; flg_update_ZN(cpu.A); }
static inline void tax(void) { cpu.X = cpu.A; flg_update_ZN(cpu.X); }
static inline void tsx(void) { cpu.X = cpu.S; flg_update_ZN(cpu.X); }
static inline void tay(void) { cpu.Y = cpu.A; flg_update_ZN(cpu.Y); }

/* Bitwise operations. */
static inline void and(void) { cpu.A &= operand; flg_update_ZN(cpu.A); }
static inline void eor(void) { cpu.A ^= operand; flg_update_ZN(cpu.A); }
static inline void ora(void) { cpu.A |= operand; flg_update_ZN(cpu.A); }

/* ADC - Add with Carry. */
static inline void adc(void) {
    /* ... */
}

/* ASL - Arithmetic Shift Left. */
static inline void asl(void) {
    /* ... */
}

/* BIT - Bit Test. */
static inline void bit(void) {
    /* ... */
}

/* BRK - Force Interrupt. */
static inline void brk(void) {
    /* ... */
}

/* CMP - Compare. */
static inline void cmp(void) {
    /* ... */
}

/* CPX - Compare X Register. */
static inline void cpx(void) {
    /* ... */
}

/* CPY - Compare Y Register. */
static inline void cpy(void) {
    /* ... */
}

/* INC - Increment Memory. */
static inline void inc(void) {
    /* ... */
}

static inline void inx(void) { cpu.X++; flg_update_ZN(cpu.X); }
static inline void iny(void) { cpu.Y++; flg_update_ZN(cpu.Y); }
static inline void dex(void) { cpu.X--; flg_update_ZN(cpu.X); }
static inline void dey(void) { cpu.Y--; flg_update_ZN(cpu.Y); }

/* DEC - Decrement Memory. */
static inline void dec(void) {
    /* ... */
}


/* JMP - Jump. */
static inline void jmp(void) {
    cpu.PC = address;
}

/* JSR - Jump to Subroutine. */
static inline void jsr(void) {
    /* ... */
}

/* LDA - Load Accumulator. */
static inline void lda(void) {
    /* ... */
}

/* LDX - Load X Register. */
static inline void ldx(void) {
    /* ... */
}

/* LDY - Load Y Register. */
static inline void ldy(void) {
    /* ... */
}

/* LSR - Logical Shift Right. */
static inline void lsr(void) {
    /* ... */
}

/* NOP - No Operation. */
static inline void nop(void) {
    /* ... */
}

/* PHA - Push Accumulator. */
static inline void pha(void) {
    /* ... */
}

/* PHP - Push Processor Status. */
static inline void php(void) {
    /* ... */
}

/* PLA - Pull Accumulator. */
static inline void pla(void) {
    /* ... */
}

/* PLP - Pull Processor Status. */
static inline void plp(void) {
    /* ... */
}

/* ROL - Rotate Left. */
static inline void rol(void) {
    /* ... */
}

/* ROR - Rotate Right. */
static inline void ror(void) {
    /* ... */
}

/* RTI - Return from Interrupt. */
static inline void rti(void) {
    /* ... */
}

/* RTS - Return from Subroutine. */
static inline void rts(void) {
    /* ... */
}

/* SBC - Subtract with Carry. */
static inline void sbc(void) {
    /* ... */
}

/* ----------------------------------------------------------------- */
/* CPU operation tables. */
/* ----------------------------------------------------------------- */

typedef void (*Function)(void);
Function cpu_instruction_table[256];
Function cpu_addressing_table[256];
int cpu_cycles_table[256];

static inline void set_instruction(byte opcode, int cycles,
        Function operation, Function mode) {
    cpu_instruction_table[opcode] = operation;
    cpu_addressing_table[opcode] = mode;
    cpu_cycles_table[opcode] = cycles;
}

static inline void init_instruction_table(void) {
    /* Reset CPU tables. */
    for (int opcode = 0; opcode < 256; opcode++) {
        set_instruction(opcode, 2, nop, implied);
    }

    /* ADC - Add with Carry. */
    set_instruction(0x69, 2, adc, immediate);
    set_instruction(0x65, 3, adc, zero_page);
    set_instruction(0x75, 4, adc, zero_page_x);
    set_instruction(0x6D, 4, adc, absolute);
    set_instruction(0x7D, 4, adc, absolute_x);
    set_instruction(0x79, 4, adc, absolute_y);
    set_instruction(0x61, 6, adc, indirect_x);
    set_instruction(0x71, 5, adc, indirect_y);

    /* AND - Logical AND. */
    set_instruction(0x29, 2, and, immediate);
    set_instruction(0x25, 3, and, zero_page);
    set_instruction(0x35, 4, and, zero_page_x);
    set_instruction(0x2D, 4, and, absolute);
    set_instruction(0x3D, 4, and, absolute_x);
    set_instruction(0x39, 4, and, absolute_y);
    set_instruction(0x21, 6, and, indirect_x);
    set_instruction(0x31, 5, and, indirect_y);

    /* ASL - Arithmetic Shift Left. */
    set_instruction(0x0A, 2, asl, accumulator);
    set_instruction(0x06, 5, asl, zero_page);
    set_instruction(0x16, 6, asl, zero_page_x);
    set_instruction(0x0E, 6, asl, absolute);
    set_instruction(0x1E, 7, asl, absolute_x);

    /* BCC - Branch if Carry Clear. */
    set_instruction(0x90, 2, bcc, relative);

    /* BCS - Branch if Carry Set. */
    set_instruction(0xB0, 2, bcs, relative);

    /* BEQ - Branch if Equal. */
    set_instruction(0xF0, 2, beq, relative);

    /* BIT - Bit Test. */
    set_instruction(0x24, 3, bit, zero_page);
    set_instruction(0x2C, 4, bit, absolute);

    /* BMI - Branch if Minus. */
    set_instruction(0x30, 2, bmi, relative);

    /* BNE - Branch if Not Equal. */
    set_instruction(0xD0, 2, bne, relative);

    /* BPL - Branch if Positive. */
    set_instruction(0x10, 2, bpl, relative);

    /* BRK - Force Interrupt. */
    set_instruction(0x00, 7, brk, implied);

    /* BVC - Branch if Overflow Clear. */
    set_instruction(0x50, 2, bvc, relative);

    /* BVS - Branch if Overflow Set. */
    set_instruction(0x70, 2, bvs, relative);

    /* CLC - Clear Carry Flag. */
    set_instruction(0x18, 2, clc, implied);

    /* CLD - Clear Decimal Mode. */
    set_instruction(0xD8, 2, cld, implied);

    /* CLI - Clear Interrupt Disable. */
    set_instruction(0x58, 2, cli, implied);

    /* CLV - Clear Overflow Flag. */
    set_instruction(0xB8, 2, clv, implied);

    /* CMP - Compare. */
    set_instruction(0xC9, 2, cmp, immediate);
    set_instruction(0xC5, 3, cmp, zero_page);
    set_instruction(0xD5, 4, cmp, zero_page_x);
    set_instruction(0xCD, 4, cmp, absolute);
    set_instruction(0xDD, 4, cmp, absolute_x);
    set_instruction(0xD9, 4, cmp, absolute_y);
    set_instruction(0xC1, 6, cmp, indirect_x);
    set_instruction(0xD1, 5, cmp, indirect_y);

    /* CPX - Compare X Register. */
    set_instruction(0xE0, 2, cpx, immediate);
    set_instruction(0xE4, 3, cpx, zero_page);
    set_instruction(0xEC, 4, cpx, absolute);

    /* CPY - Compare Y Register. */
    set_instruction(0xC0, 2, cpy, immediate);
    set_instruction(0xC4, 3, cpy, zero_page);
    set_instruction(0xCC, 4, cpy, absolute);

    /* DEC - Decrement Memory. */
    set_instruction(0xC6, 5, dec, zero_page);
    set_instruction(0xD6, 6, dec, zero_page_x);
    set_instruction(0xCE, 6, dec, absolute);
    set_instruction(0xDE, 7, dec, absolute_x);

    /* DEX - Decrement X Register. */
    set_instruction(0xCA, 2, dex, implied);

    /* DEY - Decrement Y Register. */
    set_instruction(0x88, 2, dey, implied);

    /* EOR - Exclusive OR. */
    set_instruction(0x49, 2, eor, immediate);
    set_instruction(0x45, 3, eor, zero_page);
    set_instruction(0x55, 4, eor, zero_page_x);
    set_instruction(0x4D, 4, eor, absolute);
    set_instruction(0x5D, 4, eor, absolute_x);
    set_instruction(0x59, 4, eor, absolute_y);
    set_instruction(0x41, 6, eor, indirect_x);
    set_instruction(0x51, 5, eor, indirect_y);

    /* INC - Increment Memory. */
    set_instruction(0xE6, 5, inc, zero_page);
    set_instruction(0xF6, 6, inc, zero_page_x);
    set_instruction(0xEE, 6, inc, absolute);
    set_instruction(0xFE, 7, inc, absolute_x);

    /* INX - Increment X Register. */
    set_instruction(0xE8, 2, inx, implied);

    /* INY - Increment Y Register. */
    set_instruction(0xC8, 2, iny, implied);

    /* JMP - Jump. */
    set_instruction(0x4C, 3, jmp, absolute);
    set_instruction(0x6C, 5, jmp, indirect);

    /* JSR - Jump to Subroutine. */
    set_instruction(0x20, 6, jsr, absolute);

    /* LDA - Load Accumulator. */
    set_instruction(0xA9, 2, lda, immediate);
    set_instruction(0xA5, 3, lda, zero_page);
    set_instruction(0xB5, 4, lda, zero_page_x);
    set_instruction(0xAD, 4, lda, absolute);
    set_instruction(0xBD, 4, lda, absolute_x);
    set_instruction(0xB9, 4, lda, absolute_y);
    set_instruction(0xA1, 6, lda, indirect_x);
    set_instruction(0xB1, 5, lda, indirect_y);

    /* LDX - Load X Register. */
    set_instruction(0xA2, 2, ldx, immediate);
    set_instruction(0xA6, 3, ldx, zero_page);
    set_instruction(0xB6, 4, ldx, zero_page_y);
    set_instruction(0xAE, 4, ldx, absolute);
    set_instruction(0xBE, 4, ldx, absolute_y);

    /* LDY - Load Y Register. */
    set_instruction(0xA0, 2, ldy, immediate);
    set_instruction(0xA4, 3, ldy, zero_page);
    set_instruction(0xB4, 4, ldy, zero_page_x);
    set_instruction(0xAC, 4, ldy, absolute);
    set_instruction(0xBC, 4, ldy, absolute_x);

    /* LSR - Logical Shift Right. */
    set_instruction(0x4A, 2, lsr, accumulator);
    set_instruction(0x46, 5, lsr, zero_page);
    set_instruction(0x56, 6, lsr, zero_page_x);
    set_instruction(0x4E, 6, lsr, absolute);
    set_instruction(0x5E, 7, lsr, absolute_x);

    /* NOP - No Operation. */
    set_instruction(0xEA, 2, nop, implied);

    /* ORA - Logical Inclusive OR. */
    set_instruction(0x09, 2, ora, immediate);
    set_instruction(0x05, 3, ora, zero_page);
    set_instruction(0x15, 4, ora, zero_page_x);
    set_instruction(0x0D, 4, ora, absolute);
    set_instruction(0x1D, 4, ora, absolute_x);
    set_instruction(0x19, 4, ora, absolute_y);
    set_instruction(0x01, 6, ora, indirect_x);
    set_instruction(0x11, 5, ora, indirect_y);

    /* PHA - Push Accumulator. */
    set_instruction(0x48, 3, pha, implied);

    /* PHP - Push Processor Status. */
    set_instruction(0x08, 3, php, implied);

    /* PLA - Pull Accumulator. */
    set_instruction(0x68, 4, pla, implied);

    /* PLP - Pull Processor Status. */
    set_instruction(0x28, 4, plp, implied);

    /* ROL - Rotate Left. */
    set_instruction(0x2A, 2, rol, accumulator);
    set_instruction(0x26, 5, rol, zero_page);
    set_instruction(0x36, 6, rol, zero_page_x);
    set_instruction(0x2E, 6, rol, absolute);
    set_instruction(0x3E, 7, rol, absolute_x);

    /* ROR - Rotate Right. */
    set_instruction(0x6A, 2, ror, accumulator);
    set_instruction(0x66, 5, ror, zero_page);
    set_instruction(0x76, 6, ror, zero_page_x);
    set_instruction(0x6E, 6, ror, absolute);
    set_instruction(0x7E, 7, ror, absolute_x);

    /* RTI - Return from Interrupt. */
    set_instruction(0x40, 6, rti, implied);

    /* RTS - Return from Subroutine. */
    set_instruction(0x60, 6, rts, implied);

    /* SBC - Subtract with Carry. */
    set_instruction(0xE9, 2, sbc, immediate);
    set_instruction(0xE5, 3, sbc, zero_page);
    set_instruction(0xF5, 4, sbc, zero_page_x);
    set_instruction(0xED, 4, sbc, absolute);
    set_instruction(0xFD, 4, sbc, absolute_x);
    set_instruction(0xF9, 4, sbc, absolute_y);
    set_instruction(0xE1, 6, sbc, indirect_x);
    set_instruction(0xF1, 5, sbc, indirect_y);

    /* SEC - Set Carry Flag. */
    set_instruction(0x38, 2, sec, implied);

    /* SEC - Set Decimal Flag. */
    set_instruction(0xF8, 2, sed, implied);

    /* SEI - Set Interrupt Disable. */
    set_instruction(0x78, 2, sei, implied);

    /* STA - Store Accumulator. */
    set_instruction(0x85, 3, sta, zero_page);
    set_instruction(0x95, 4, sta, zero_page_x);
    set_instruction(0x8D, 4, sta, absolute);
    set_instruction(0x9D, 5, sta, absolute_x);
    set_instruction(0x99, 5, sta, absolute_y);
    set_instruction(0x81, 6, sta, indirect_x);
    set_instruction(0x91, 6, sta, indirect_y);

    /* STX - Store X Register. */
    set_instruction(0x86, 3, stx, zero_page);
    set_instruction(0x96, 4, stx, zero_page_y);
    set_instruction(0x8E, 4, stx, absolute);

    /* STY - Store Y Register. */
    set_instruction(0x84, 3, sty, zero_page);
    set_instruction(0x94, 4, sty, zero_page_x);
    set_instruction(0x8C, 4, sty, absolute);

    /* TAX - Transfer Accumulator to X. */
    set_instruction(0xAA, 2, tax, implied);

    /* TAY - Transfer Accumulator to Y. */
    set_instruction(0xA8, 2, tay, implied);

    /* TSX - Transfer Stack Pointer to X. */
    set_instruction(0xBA, 2, tsx, implied);

    /* TSA - Transfer X to Accumulator. */
    set_instruction(0x8A, 2, txa, implied);

    /* TXS - Transfer X to Stack Pointer. */
    set_instruction(0x9A, 2, txs, implied);

    /* TYA - Transfer Y to Accumulator. */
    set_instruction(0x98, 2, tya, implied);
}

/* -----------------------------------------------------------------
 * CPU iterface.
 * -------------------------------------------------------------- */

void cpu_reset(void) {
    cpu = (CPU) { 0x0000, 0x00, 0x00, 0x00, 0x00 };
    wait_cycles = 0;
}

void cpu_init(void) {
    init_instruction_table();
    cpu_reset();
}

void cpu_cycle(int num_cycles) {
    while (num_cycles > 0) {
        if (wait_cycles == 0) {
            extra_cycles = 0;
            byte opcode = mem_read_byte(cpu.PC);
            (*cpu_addressing_table[opcode])();
            (*cpu_instruction_table[opcode])();
            wait_cycles = cpu_cycles_table[opcode] + extra_cycles - 1;
        }
        else {
            wait_cycles--;
        }
        num_cycles--;
    }
}
