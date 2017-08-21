#include <stdio.h>  /* For testing. */
#include "../include/common.h"
#include "../include/cpu.h"
#include "../include/cpu_flags.h"
#include "../include/memory.h"

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
 * Stack operations.
 * -------------------------------------------------------------- */

static inline void push(byte data) {
    mem_write(0x0100 | cpu.S--, data);
}

static inline void push_address(word data) {
    mem_write(0x0100 | cpu.S--, data >> 8);
    mem_write(0x0100 | cpu.S--, data & 0xFF);
}

static inline byte pop(void) {
    return mem_read_8(0x0100 | ++cpu.S);
}

static inline word pop_address(void) {
    word result = mem_read_16(0x0100 | ++cpu.S);
    cpu.S++;
    return result;
}

/* -----------------------------------------------------------------
 * CPU addressing modes.
 * -------------------------------------------------------------- */

static inline bool is_diff_page(word address1, word address2) {
    return (address1 & 0xFF00) != (address2 & 0xFF00);
}

static inline void absolute(void) {
    address = mem_read_16(cpu.PC + 1);
    cpu.PC += 3;
}

static inline void absolute_x(void) {
    address = mem_read_16(cpu.PC + 1) + cpu.X;
    extra_cycles = is_diff_page(address, address - cpu.X);
    cpu.PC += 3;
}

static inline void absolute_y(void) {
    address = mem_read_16(cpu.PC + 1) + cpu.Y;
    extra_cycles = is_diff_page(address, address - cpu.Y);
    cpu.PC += 3;
}

static inline void accumulator(void) {
    cpu.PC += 1;
}

static inline void immediate(void) {
    address = cpu.PC + 1;
    cpu.PC += 2;
}

static inline void implied(void) {
    cpu.PC += 1;
}

static inline void indirect(void) {
    address = mem_read_16(cpu.PC + 1);
    if (address & 0xFF == 0xFF) {
        byte lo = mem_read_8(address);
        byte hi = mem_read_8(address - 0xFF);
        address = (hi << 8) | lo;
    }
    else {
        address = mem_read_16(address);
    }
}

static inline void indirect_x(void) {
    address = mem_read_8 (cpu.PC + 1);
    address = mem_read_16((address + cpu.X) & 0xFF);
    cpu.PC += 2;
}

static inline void indirect_y(void) {
    address = mem_read_8 (cpu.PC + 1);
    address = mem_read_16(address) + cpu.Y;
    extra_cycles = is_diff_page(address, address - cpu.Y);
    cpu.PC += 2;
}

static inline void relative(void) {
    address = cpu.PC + 1;
    cpu.PC += 2;
}

static inline void zero_page(void) {
    address = mem_read_8(cpu.PC + 1);
    cpu.PC += 2;
}

static inline void zero_page_x(void) {
    address = (mem_read_8(cpu.PC + 1) + cpu.X) & 0xFF;
    cpu.PC += 2;
}

static inline void zero_page_y(void) {
    address = (mem_read_8(cpu.PC + 1) + cpu.Y) & 0xFF;
    cpu.PC += 2;
}

/* -----------------------------------------------------------------
 * CPU instructions.
 * -------------------------------------------------------------- */

/* Branching instructions. */
static inline void branch(bool condition) {
    if (condition) {
        operand = mem_read_8(address);
        address = cpu.PC;
        cpu.PC += (int8_t) operand;

        extra_cycles++;
        if (is_diff_page(address, cpu.PC)) {
            extra_cycles++;  /* Page crossed. */
        }
    }
}

/* ADC - Add with Carry. */
static inline void adc(void) {
    operand = mem_read_8(address);
    int result = cpu.A + operand + flg_is_C();
    flg_update_ZN(result);
    flg_update_C (result, ADC);
    flg_update_V (result, cpu.A, operand);
    cpu.A = result & 0xFF;
}

/* AND - Logical AND. */
static inline void and(void) {
    operand = mem_read_8(address);
    flg_update_ZN(cpu.A &= operand);
}

/* ASL - Arithmetic Shift Left (Accumulator). */
static inline void asl_a(void) {
    flg_update_C (cpu.A, ROL);
    flg_update_ZN(cpu.A <<= 1);
}

/* ASL - Arithmetic Shift Left (Memory). */
static inline void asl_m(void) {
    operand = mem_read_8(address);
    flg_update_C (operand, ROL);
    mem_write(address, operand <<= 1);
    flg_update_ZN(operand);
    extra_cycles = 0;
}

/* BCC - Branch if Carry Clear. */
static inline void bcc(void) {
    branch(!flg_is_C());
}

/* BCS - Branch if Carry Set. */
static inline void bcs(void) {
    branch(flg_is_C());
}

/* BEQ - Branch if Equal. */
static inline void beq(void) {
    branch(flg_is_Z());
}

/* BIT - Bit Test. */
static inline void bit(void) {
    operand = mem_read_8(address);
    byte test = cpu.A & operand;
    flg_update_ZN(test);
    flg_update_V_bit(test);
}

/* BMI - Branch if Minus. */
static inline void bmi(void) {
    branch(flg_is_N());
}

/* BNE - Branch if Not Equal. */
static inline void bne(void) {
    branch(!flg_is_Z());
}

/* BPL - Branch if Positive. */
static inline void bpl(void) {
    branch(!flg_is_N());
}

/* BRK - Force Interrupt. */
static inline void brk(void) {
    push_address(cpu.PC + 1);
    push(flg_get_status(true));
    cpu.PC = mem_read_16(0xFFFE);
    flg_set_I();
}

/* BVC - Branch if Carry Clear. */
static inline void bvc(void) {
    branch(!flg_is_V());
}

/* BVS - Branch if Carry Set. */
static inline void bvs(void) {
    branch(flg_is_V());
}

/* CLC - Clear Carry Flag. */
static inline void clc(void) {
    flg_clear_C();
}

/* CLD - Clear Decimal Mode. */
static inline void cld(void) {
    flg_clear_D();
}

/* CLI - Clear Interrupt Disable. */
static inline void cli(void) {
    flg_clear_I();
}

/* CLV - Clear Overflow Flag. */
static inline void clv(void) {
    flg_clear_V();
}

/* CMP - Compare. */
static inline void cmp(void) {
    operand = mem_read_8(address);
    flg_update_C (cpu.A - operand, CMP);
    flg_update_ZN(cpu.A - operand);
}

/* CPX - Compare X Register. */
static inline void cpx(void) {
    operand = mem_read_8(address);
    flg_update_C (cpu.X - operand, CMP);
    flg_update_ZN(cpu.X - operand);
}

/* CPY - Compare Y Register. */
static inline void cpy(void) {
    operand = mem_read_8(address);
    flg_update_C (cpu.Y - operand, CMP);
    flg_update_ZN(cpu.Y - operand);
}

/* DEC - Decrement Memory. */
static inline void dec(void) {
    operand = mem_read_8(address);
    mem_write(address, --operand);
    flg_update_ZN(operand);
    extra_cycles = 0;
}

/* DEX - Decrement X Register. */
static inline void dex(void) {
    flg_update_ZN(--cpu.X);
}

/* DEY - Decrement Y Register. */
static inline void dey(void) {
    flg_update_ZN(--cpu.Y);
}

/* EOR - Exclusive OR. */
static inline void eor(void) {
    operand = mem_read_8(address);
    flg_update_ZN(cpu.A ^= operand);
}

/* INC - Increment Memory. */
static inline void inc(void) {
    operand = mem_read_8(address);
    mem_write(address, ++operand);
    flg_update_ZN(operand);
    extra_cycles = 0;
}

/* INX - Increment X Register. */
static inline void inx(void) {
    flg_update_ZN(++cpu.X);
}

/* INY - Increment Y Register. */
static inline void iny(void) {
    flg_update_ZN(++cpu.Y);
}

/* JMP - Jump. */
static inline void jmp(void) {
    cpu.PC = address;
}

/* JSR - Jump to Subroutine. */
static inline void jsr(void) {
    push_address(cpu.PC);
    cpu.PC = address;
}

/* LDA - Load Accumulator. */
static inline void lda(void) {
    operand = mem_read_8(address);
    flg_update_ZN(cpu.A = operand);
}

/* LDX - Load X Register. */
static inline void ldx(void) {
    operand = mem_read_8(address);
    flg_update_ZN(cpu.X = operand);
}

/* LDY - Load Y Register. */
static inline void ldy(void) {
    operand = mem_read_8(address);
    flg_update_ZN(cpu.Y = operand);
}

/* LSR - Logical Shift Right (Accumulator). */
static inline void lsr_a(void) {
    flg_update_C (cpu.A, ROR);
    flg_update_ZN(cpu.A >>= 1);
}

/* LSR - Logical Shift Right (Memory). */
static inline void lsr_m(void) {
    operand = mem_read_8(address);
    flg_update_C (operand, ROR);
    mem_write(address, operand >>= 1);
    flg_update_ZN(operand);
    extra_cycles = 0;
}

/* NOP - No Operation. */
static inline void nop(void) {}

/* ORA - Logical Inclusive OR. */
static inline void ora(void) {
    operand = mem_read_8(address);
    flg_update_ZN(cpu.A |= operand);
}

/* PHA - Push Accumulator. */
static inline void pha(void) {
    push(cpu.A);
}

/* PHP - Push Processor Status. */
static inline void php(void) {
    push(flg_get_status(true));
}

/* PLA - Pull Accumulator. */
static inline void pla(void) {
    cpu.A = pop();
}

/* PLP - Pull Processor Status. */
static inline void plp(void) {
    flg_set_status(pop());
}

/* ROL - Rotate Left (Accumulator). */
static inline void rol_a(void) {
    flg_update_C (cpu.A, ROL);
    cpu.A = (cpu.A << 1) | flg_is_C();
    flg_update_ZN(cpu.A);
}

/* ROL - Rotate Left (Memory). */
static inline void rol_m(void) {
    operand = mem_read_8(address);
    flg_update_C (operand, ROL);
    operand = (operand << 1) | flg_is_C();
    mem_write(address, operand);
    flg_update_ZN(operand);
    extra_cycles = 0;
}

/* ROR - Rotate Right (Accumulator). */
static inline void ror_a(void) {
    flg_update_C (cpu.A, ROR);
    cpu.A = (cpu.A >> 1) | (flg_is_C() << 7);
    flg_update_ZN(cpu.A);
}

/* ROR - Rotate Right (Memory). */
static inline void ror_m(void) {
    operand = mem_read_8(address);
    flg_update_C (operand, ROR);
    operand = (operand >> 1) | (flg_is_C() << 7);
    mem_write(address, operand);
    flg_update_ZN(operand);
    extra_cycles = 0;
}

/* RTI - Return from Interrupt. */
static inline void rti(void) {
    flg_set_status(pop());
    cpu.PC = pop_address();
}

/* RTS - Return from Subroutine. */
static inline void rts(void) {
    cpu.PC = pop_address();
}

/* SBC - Subtract with Carry. */
static inline void sbc(void) {
    operand = mem_read_8(address);
    int result = (int8_t) cpu.A - (int8_t) operand - (int8_t) !flg_is_C();
    flg_update_ZN(result);
    flg_update_C (result, SBC);
    byte a = cpu.A   == 0 ? 0xFF : cpu.A;
    byte b = operand == 0 ? 0xFF : -operand;
    flg_update_V (result, a, b);
    cpu.A = result;
}

/* SEC - Set Carry Flag. */
static inline void sec(void) {
    flg_set_C();
}

/* SED - Set Decimal Flag. */
static inline void sed(void) {
    flg_set_D();
}

/* SEI - Set Interrupt Disable. */
static inline void sei(void) {
    flg_set_I();
}

/* STA - Store Accumulator. */
static inline void sta(void) {
    mem_write(address, cpu.A);
    extra_cycles = 0;
}

/* STX - Store X Register. */
static inline void stx(void) {
    mem_write(address, cpu.X);
}

/* STY - Store Y Register. */
static inline void sty(void) {
    mem_write(address, cpu.Y);
}

/* TAX - Transfer Accumulator to X. */
static inline void tax(void) {
    flg_update_ZN(cpu.X = cpu.A);
}

/* TAY - Transfer Accumulator to Y. */
static inline void tay(void) {
    flg_update_ZN(cpu.Y = cpu.A);
}

/* TSX - Transfer Stack Pointer to X. */
static inline void tsx(void) {
    flg_update_ZN(cpu.X = cpu.S);
}

/* Transfer X to Accumulator. */
static inline void txa(void) {
    flg_update_ZN(cpu.A = cpu.X);
}

/* TXS - Transfer X to Stack Pointer. */
static inline void txs(void) {
    cpu.S = cpu.X;
}

/* TYA - Transfer Y to Accumulator. */
static inline void tya(void) {
    flg_update_ZN(cpu.A = cpu.Y);
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
    set_instruction(0x0A, 2, asl_a, accumulator);
    set_instruction(0x06, 5, asl_m, zero_page);
    set_instruction(0x16, 6, asl_m, zero_page_x);
    set_instruction(0x0E, 6, asl_m, absolute);
    set_instruction(0x1E, 7, asl_m, absolute_x);

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
    set_instruction(0x4A, 2, lsr_a, accumulator);
    set_instruction(0x46, 5, lsr_m, zero_page);
    set_instruction(0x56, 6, lsr_m, zero_page_x);
    set_instruction(0x4E, 6, lsr_m, absolute);
    set_instruction(0x5E, 7, lsr_m, absolute_x);

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
    set_instruction(0x2A, 2, rol_a, accumulator);
    set_instruction(0x26, 5, rol_m, zero_page);
    set_instruction(0x36, 6, rol_m, zero_page_x);
    set_instruction(0x2E, 6, rol_m, absolute);
    set_instruction(0x3E, 7, rol_m, absolute_x);

    /* ROR - Rotate Right. */
    set_instruction(0x6A, 2, ror_a, accumulator);
    set_instruction(0x66, 5, ror_m, zero_page);
    set_instruction(0x76, 6, ror_m, zero_page_x);
    set_instruction(0x6E, 6, ror_m, absolute);
    set_instruction(0x7E, 7, ror_m, absolute_x);

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
    cpu = (CPU) { 0x0000, 0xFF, 0x00, 0x00, 0x00 };
    wait_cycles = 0;
    flg_reset();
}

void cpu_init(void) {
    init_instruction_table();
    cpu_reset();
}

void cpu_cycle(int num_cycles) {
    while (num_cycles > 0) {
        if (wait_cycles == 0) {
            extra_cycles = 0;
            byte opcode = mem_read_8(cpu.PC);
            (*cpu_addressing_table[opcode])();
            (*cpu_instruction_table[opcode])();
            wait_cycles = cpu_cycles_table[opcode] + extra_cycles;
        }

        wait_cycles--;
        num_cycles--;
    }
}
