#define CPU_LOGGING

#include <stdio.h>
#include "../include/common.h"
#include "../include/cpu.h"
#include "../include/cpu_flags.h"
#include "../include/cpu_internal.h"
#include "../include/cpu_logging.h"
#include "../include/log.h"
#include "../include/memory.h"

/* -----------------------------------------------------------------
 * CPU variables.
 * -------------------------------------------------------------- */

typedef void (*Function)(void);
static Function cpu_instruction_table[256];
static Function cpu_addressing_table[256];

CPU cpu;                    /* CPU status. */

static bool nmi = false;    /* NMI interrupt. */

static byte opcode;         /* Current opcode being executed. */
static byte operand;        /* Operand (8 bit) of the instruction. */
static word address;        /* Operand (16 bit) of the instruction. */
static byte lo, hi;         /* Temporary variables low/high byte. */

unsigned long long cycles;  /* Total number of cycles run so far. */

static bool initialized_table = false;

/* -----------------------------------------------------------------
 * CPU addressing modes.
 * -------------------------------------------------------------- */

static inline bool is_diff_page(word address1, word address2) {
    return (address1 & 0xFF00) != (address2 & 0xFF00);
}

static inline void absolute(void) {
    address = cpu_fetch_16();
    operand = cpu_read(address);
}

static inline void absolute_write(void) {
    address = cpu_fetch_16();
}

static inline void absolute_jump(void) {
    lo = cpu_fetch();
}

static inline void absolute_x(void) {
    lo = cpu_fetch(); hi = cpu_fetch();
    address = (hi << 8) | ((lo + cpu.X) & 0xFF);
    operand = cpu_read(address);

    address = ((hi << 8) | lo) + cpu.X;
    if (is_diff_page(address, address - cpu.X)) {
        operand = cpu_read(address);
    }
}

static inline void absolute_x_modify(void) {
    lo = cpu_fetch(); hi = cpu_fetch();
    address = (hi << 8) | ((lo + cpu.X) & 0xFF);
    operand = cpu_read(address);
    address = ((hi << 8) | lo) + cpu.X;
    operand = cpu_read(address);
}

static inline void absolute_x_write(void) {
    lo = cpu_fetch(); hi = cpu_fetch();
    address = (hi << 8) | ((lo + cpu.X) & 0xFF);
    operand = cpu_read(address);
    address = ((hi << 8) | lo) + cpu.X;
}

static inline void absolute_y(void) {
    lo = cpu_fetch(); hi = cpu_fetch();
    address = (hi << 8) | ((lo + cpu.Y) & 0xFF);
    operand = cpu_read(address);

    address = ((hi << 8) | lo) + cpu.Y;
    if (is_diff_page(address, address - cpu.Y)) {
        operand = cpu_read(address);
    }
}

static inline void absolute_y_write(void) {
    lo = cpu_fetch(); hi = cpu_fetch();
    address = (hi << 8) | ((lo + cpu.Y) & 0xFF);
    operand = cpu_read(address);
    address = ((hi << 8) | lo) + cpu.Y;
}

static inline void accumulator(void) {
    cpu_fetch_dummy(); /* Read next instruction byte and throw it away. */
}

static inline void immediate(void) {
    operand = cpu_fetch();
}

static inline void implied(void) {
    cpu_fetch_dummy(); /* Read next instruction byte and throw it away. */
}

static inline void indirect(void) {
    address = cpu_fetch_16();

    if (lo == 0xFF) {
        lo = cpu_read(address);
        hi = cpu_read(address - 0xFF);
        address = (hi << 8) | lo;
    }
    else {
        address = cpu_read_16(address);
    }
}

static inline void indirect_x(void) {
    address = cpu_fetch();
    cpu_read(address);
    lo = cpu.ram[(address + cpu.X) & 0xFF];
    hi = cpu.ram[(address + cpu.X + 1) & 0xFF];
    address = (hi << 8) | lo;
    cycles += 2;
    operand = cpu_read(address);
}

static inline void indirect_x_write(void) {
    address = cpu_fetch();
    cpu_read(address);
    lo = cpu.ram[(address + cpu.X) & 0xFF];
    hi = cpu.ram[(address + cpu.X + 1) & 0xFF];
    address = (hi << 8) | lo;
    cycles += 2;
}

static inline void indirect_y(void) {
    address = cpu_fetch();
    lo = cpu.ram[address];
    hi = cpu.ram[(address + 1) & 0xFF];
    address = (hi << 8) | ((lo + cpu.Y) & 0xFF);
    cycles += 2;
    operand = cpu_read(address);

    address = ((hi << 8) | lo) + cpu.Y;
    if (is_diff_page(address, address - cpu.Y)) {
        operand = cpu_read(address);
    }
}

static inline void indirect_y_write(void) {
    address = cpu_fetch();
    lo = cpu.ram[address];
    hi = cpu.ram[(address + 1) & 0xFF];
    address = (hi << 8) | ((lo + cpu.Y) & 0xFF);
    cycles += 2;
    operand = cpu_read(address);
    address = ((hi << 8) | lo) + cpu.Y;
}

static inline void relative(void) {
    operand = cpu_fetch();
}

static inline void zero_page(void) {
    address = cpu_fetch();
    operand = cpu_read(address);
}

static inline void zero_page_write(void) {
    address = cpu_fetch();
}

static inline void zero_page_x(void) {
    address = cpu_fetch();
    cpu_read(address);
    address = (address + cpu.X) & 0xFF;
    operand = cpu_read(address);
}

static inline void zero_page_x_write(void) {
    address = cpu_fetch();
    cpu_read(address);
    address = (address + cpu.X) & 0xFF;
}

static inline void zero_page_y(void) {
    address = cpu_fetch();
    cpu_read(address);
    address = (address + cpu.Y) & 0xFF;
    operand = cpu_read(address);
}

static inline void zero_page_y_write(void) {
    address = cpu_fetch();
    cpu_read(address);
    address = (address + cpu.Y) & 0xFF;
}

/* -----------------------------------------------------------------
 * CPU instructions.
 * -------------------------------------------------------------- */

/* Invalid / unimplemented instruction. */
static inline void invalid(void) {
    LOG_ERROR("Invalid opcode %02X at %04X.", opcode, cpu.PC - 1);
}

/* Branching instructions. */
static inline void branch(bool condition) {
    if (condition) {
        address = cpu.PC;
        cpu.PC += (int8_t) operand;

        cycles++;
        if (is_diff_page(address, cpu.PC)) {
            cycles++;  /* Page crossed. */
        }
    }
}

/* ADC - Add with Carry. */
static inline void adc(void) {
    int result = cpu.A + operand + flg_is_C();
    flg_update_ZN(result);
    flg_update_C (result, ADC);
    flg_update_V (result, cpu.A, operand);
    cpu.A = result & 0xFF;
}

/* AND - Logical AND. */
static inline void and(void) {
    flg_update_ZN(cpu.A &= operand);
}

/* ASL - Arithmetic Shift Left (Accumulator). */
static inline void asl_a(void) {
    flg_update_C (cpu.A, ROL);
    flg_update_ZN(cpu.A <<= 1);
}

/* ASL - Arithmetic Shift Left (Memory). */
static inline void asl_m(void) {
    flg_update_C (operand, ROL);
    cpu_write(address, operand);
    cpu_write(address, operand <<= 1);
    flg_update_ZN(operand);
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
    flg_update_Z(cpu.A & operand);
    flg_update_N(operand);
    flg_update_V_bit(operand);
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
    cpu_push_address(cpu.PC);
    cpu_push(flg_get_status(true));
    cpu.PC = cpu_read_16(IRQ_VECTOR);
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
    flg_update_C (cpu.A - operand, CMP);
    flg_update_ZN(cpu.A - operand);
}

/* CPX - Compare X Register. */
static inline void cpx(void) {
    flg_update_C (cpu.X - operand, CMP);
    flg_update_ZN(cpu.X - operand);
}

/* CPY - Compare Y Register. */
static inline void cpy(void) {
    flg_update_C (cpu.Y - operand, CMP);
    flg_update_ZN(cpu.Y - operand);
}

/* DEC - Decrement Memory. */
static inline void dec(void) {
    cpu_write(address, operand);
    cpu_write(address, --operand);
    flg_update_ZN(operand);
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
    flg_update_ZN(cpu.A ^= operand);
}

/* INC - Increment Memory. */
static inline void inc(void) {
    cpu_write(address, operand);
    cpu_write(address, ++operand);
    flg_update_ZN(operand);
}

/* INX - Increment X Register. */
static inline void inx(void) {
    flg_update_ZN(++cpu.X);
}

/* INY - Increment Y Register. */
static inline void iny(void) {
    flg_update_ZN(++cpu.Y);
}

/* JMP - Jump, Absolute. */
static inline void jmp_absolute(void) {
    hi = cpu_fetch();
    cpu.PC = (hi << 8) | lo;
}

/* JMP - Jump, Indirect. */
static inline void jmp_indirect(void) {
    cpu.PC = address;
}

/* JSR - Jump to Subroutine. */
static inline void jsr(void) {
    cycles++;
    cpu_push_address(cpu.PC);
    hi = cpu_fetch();
    cpu.PC = (hi << 8) | lo;
}

/* LDA - Load Accumulator. */
static inline void lda(void) {
    flg_update_ZN(cpu.A = operand);
}

/* LDX - Load X Register. */
static inline void ldx(void) {
    flg_update_ZN(cpu.X = operand);
}

/* LDY - Load Y Register. */
static inline void ldy(void) {
    flg_update_ZN(cpu.Y = operand);
}

/* LSR - Logical Shift Right (Accumulator). */
static inline void lsr_a(void) {
    flg_update_C (cpu.A, ROR);
    flg_update_ZN(cpu.A >>= 1);
}

/* LSR - Logical Shift Right (Memory). */
static inline void lsr_m(void) {
    flg_update_C (operand, ROR);
    cpu_write(address, operand);
    cpu_write(address, operand >>= 1);
    flg_update_ZN(operand);
}

/* NOP - No Operation. */
static inline void nop(void) {}

/* ORA - Logical Inclusive OR. */
static inline void ora(void) {
    flg_update_ZN(cpu.A |= operand);
}

/* PHA - Push Accumulator. */
static inline void pha(void) {
    cpu_push(cpu.A);
}

/* PHP - Push Processor Status. */
static inline void php(void) {
    cpu_push(flg_get_status(true));
}

/* PLA - Pull Accumulator. */
static inline void pla(void) {
    cycles++; /* Increment S cycle. */
    flg_update_ZN(cpu.A = cpu_pop());
}

/* PLP - Pull Processor Status. */
static inline void plp(void) {
    cycles++; /* Increment S cycle. */
    flg_set_status(cpu_pop());
}

/* ROL - Rotate Left (Accumulator). */
static inline void rol_a(void) {
    flg_update_C (cpu.A, ROL);
    cpu.A = (cpu.A << 1) | flg_is_C();
    flg_update_ZN(cpu.A);
}

/* ROL - Rotate Left (Memory). */
static inline void rol_m(void) {
    flg_update_C (operand, ROL);
    cpu_write(address, operand);
    operand = (operand << 1) | flg_is_C();
    cpu_write(address, operand);
    flg_update_ZN(operand);
}

/* ROR - Rotate Right (Accumulator). */
static inline void ror_a(void) {
    bool carry = flg_is_C();
    flg_update_C (cpu.A, ROR);
    cpu.A = (cpu.A >> 1) | (carry << 7);
    flg_update_ZN(cpu.A);
}

/* ROR - Rotate Right (Memory). */
static inline void ror_m(void) {
    bool carry = flg_is_C();
    flg_update_C (operand, ROR);
    cpu_write(address, operand);
    operand = (operand >> 1) | (carry << 7);
    cpu_write(address, operand);
    flg_update_ZN(operand);
}

/* RTI - Return from Interrupt. */
static inline void rti(void) {
    cycles++; /* Increment S cycle. */
    flg_set_status(cpu_pop());
    cpu.PC = cpu_pop_address();
}

/* RTS - Return from Subroutine. */
static inline void rts(void) {
    cycles++; /* Increment S cycle. */
    cpu.PC = cpu_pop_address() + 1;
    cycles++; /* Increment PC cycle. */
}

/* SBC - Subtract with Carry. */
static inline void sbc(void) {
    operand ^= 0xFF;
    int result = cpu.A + operand + flg_is_C();
    flg_update_ZN(result);
    flg_update_C (result, ADC);
    flg_update_V (result, cpu.A, operand);
    cpu.A = result & 0xFF;
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
    cpu_write(address, cpu.A);
}

/* STX - Store X Register. */
static inline void stx(void) {
    cpu_write(address, cpu.X);
}

/* STY - Store Y Register. */
static inline void sty(void) {
    cpu_write(address, cpu.Y);
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

/* -----------------------------------------------------------------
 * Unofficial CPU instructions.
 * -------------------------------------------------------------- */

/* ALR - AND and Shift Right. */
static inline void alr(void) {
    cpu.A &= operand;
    flg_update_C (cpu.A, ROR);
    flg_update_ZN(cpu.A >>= 1);
}

/* ANC - AND with Carry */
static inline void anc(void) {
    flg_update_C (cpu.A, ROL);
    flg_update_ZN(cpu.A &= operand);
}

/* ARR - AND and Rotate Right. */
static inline void arr(void) {
    cpu.A &= operand;

    /* Set the V-flag according to (A and #{imm}) + #{imm}. */
    flg_update_V (cpu.A + operand, cpu.A, operand);

    bool carry = flg_is_C();
    flg_update_C (cpu.A, ROL);
    cpu.A = (cpu.A >> 1) | (carry << 7);
    flg_update_ZN(cpu.A);
}

/* AXS - AND X with Accumulator and Subtract. */
static inline void axs(void) {
    cpu.X &= cpu.A;
    cpu.X -= operand;
    flg_update_C (cpu.X, CMP);
    flg_update_ZN(cpu.X);
}

/* DCP - Decrement and compare. */
static inline void dcp(void) {
    cpu_write(address, operand);
    cpu_write(address, --operand);
    flg_update_C (cpu.A - operand, CMP);
    flg_update_ZN(cpu.A - operand);
}

/* LAX - Load Accumulator and X. */
static inline void lax(void) {
    flg_update_ZN(cpu.A = cpu.X = operand);
}

/* HLT - Halt. */
static inline void hlt(void) {
    LOG_ERROR("CPU halted, opcode %02X at %04X.", opcode, cpu.PC);
}

/* ISB - Increment and Subtract. */
static inline void isb(void) {
    cpu_write(address, operand);
    cpu_write(address, ++operand);

    operand ^= 0xFF;
    int result = cpu.A + operand + flg_is_C();
    flg_update_ZN(result);
    flg_update_C (result, ADC);
    flg_update_V (result, cpu.A, operand);
    cpu.A = result & 0xFF;
}

/* RLA - Rotate Left and AND. */
static inline void rla(void) {
    bool carry = flg_is_C();
    flg_update_C (operand, ROL);
    cpu_write(address, operand);
    operand = (operand << 1) | carry;
    cpu_write(address, operand);
    flg_update_ZN(cpu.A &= operand);
}

/* RRA - Rotate Right and Add. */
static inline void rra(void) {
    bool carry = flg_is_C();
    flg_update_C (operand, ROR);
    cpu_write(address, operand);
    operand = (operand >> 1) | (carry << 7);
    cpu_write(address, operand);

    int result = cpu.A + operand + flg_is_C();
    flg_update_ZN(result);
    flg_update_C (result, ADC);
    flg_update_V (result, cpu.A, operand);
    cpu.A = result & 0xFF;
}

/* SAX - Store Accumulator AND X. */
static inline void sax(void) {
    cpu_write(address, cpu.A & cpu.X);
}

/* SLO - Shift Left and Inclusive OR. */
static inline void slo(void) {
    flg_update_C (operand, ROL);
    cpu_write(address, operand);
    cpu_write(address, operand <<= 1);
    flg_update_ZN(cpu.A |= operand);
}

/* SRE - Shift Right and Exclusive OR. */
static inline void sre(void) {
    flg_update_C (operand, ROR);
    cpu_write(address, operand);
    cpu_write(address, operand >>= 1);
    flg_update_ZN(cpu.A ^= operand);
}

/* XAA - Transfer X to Accumulator and AND. */
static inline void xaa(void) {
    cpu.A = cpu.X & operand;
    flg_update_ZN(cpu.A);
}

/* --------------------------------------------------------------------
 * CPU operation tables.
 * ----------------------------------------------------------------- */

#define SET_INSTRUCTION(opcode, name, oper, mode)   \
    cpu_instruction_table [opcode] = oper;          \
    cpu_addressing_table  [opcode] = mode;          \
    cpu_log_set_function  (opcode, cpu_log_##mode); \
    cpu_log_set_name      (opcode, name)

static inline void init_instruction_table(void) {
    /* Reset CPU tables. */
    for (int opcode = 0; opcode < 256; opcode++) {
        SET_INSTRUCTION(opcode, "****", invalid, implied);
    }

    /* ADC - Add with Carry. */
    SET_INSTRUCTION(0x69, " ADC", adc, immediate);
    SET_INSTRUCTION(0x65, " ADC", adc, zero_page);
    SET_INSTRUCTION(0x75, " ADC", adc, zero_page_x);
    SET_INSTRUCTION(0x6D, " ADC", adc, absolute);
    SET_INSTRUCTION(0x7D, " ADC", adc, absolute_x);
    SET_INSTRUCTION(0x79, " ADC", adc, absolute_y);
    SET_INSTRUCTION(0x61, " ADC", adc, indirect_x);
    SET_INSTRUCTION(0x71, " ADC", adc, indirect_y);

    /* ALR - AND and Shift Right. */
    SET_INSTRUCTION(0x4B, "*ALR", alr, immediate);

    /* ANC - AND with Carry. */
    SET_INSTRUCTION(0x0B, "*ANC", anc, immediate);
    SET_INSTRUCTION(0x2B, "*ANC", anc, immediate);

    /* AND - Logical AND. */
    SET_INSTRUCTION(0x29, " AND", and, immediate);
    SET_INSTRUCTION(0x25, " AND", and, zero_page);
    SET_INSTRUCTION(0x35, " AND", and, zero_page_x);
    SET_INSTRUCTION(0x2D, " AND", and, absolute);
    SET_INSTRUCTION(0x3D, " AND", and, absolute_x);
    SET_INSTRUCTION(0x39, " AND", and, absolute_y);
    SET_INSTRUCTION(0x21, " AND", and, indirect_x);
    SET_INSTRUCTION(0x31, " AND", and, indirect_y);

    /* ARR - AND and Rotate Right. */
    SET_INSTRUCTION(0x6B, "*ARR", arr, immediate);

    /* ASL - Arithmetic Shift Left. */
    SET_INSTRUCTION(0x0A, " ASL", asl_a, accumulator);
    SET_INSTRUCTION(0x06, " ASL", asl_m, zero_page);
    SET_INSTRUCTION(0x16, " ASL", asl_m, zero_page_x);
    SET_INSTRUCTION(0x0E, " ASL", asl_m, absolute);
    SET_INSTRUCTION(0x1E, " ASL", asl_m, absolute_x_modify);

    /* AXS - AND X with Accumulator and Subtract. */
    SET_INSTRUCTION(0xCB, "*AXS", axs, immediate);

    /* BCC - Branch if Carry Clear. */
    SET_INSTRUCTION(0x90, " BCC", bcc, relative);

    /* BCS - Branch if Carry Set. */
    SET_INSTRUCTION(0xB0, " BCS", bcs, relative);

    /* BEQ - Branch if Equal. */
    SET_INSTRUCTION(0xF0, " BEQ", beq, relative);

    /* BIT - Bit Test. */
    SET_INSTRUCTION(0x24, " BIT", bit, zero_page);
    SET_INSTRUCTION(0x2C, " BIT", bit, absolute);

    /* BMI - Branch if Minus. */
    SET_INSTRUCTION(0x30, " BMI", bmi, relative);

    /* BNE - Branch if Not Equal. */
    SET_INSTRUCTION(0xD0, " BNE", bne, relative);

    /* BPL - Branch if Positive. */
    SET_INSTRUCTION(0x10, " BPL", bpl, relative);

    /* BRK - Force Interrupt. */
    SET_INSTRUCTION(0x00, " BRK", brk, implied);

    /* BVC - Branch if Overflow Clear. */
    SET_INSTRUCTION(0x50, " BVC", bvc, relative);

    /* BVS - Branch if Overflow Set. */
    SET_INSTRUCTION(0x70, " BVS", bvs, relative);

    /* CLC - Clear Carry Flag. */
    SET_INSTRUCTION(0x18, " CLC", clc, implied);

    /* CLD - Clear Decimal Mode. */
    SET_INSTRUCTION(0xD8, " CLD", cld, implied);

    /* CLI - Clear Interrupt Disable. */
    SET_INSTRUCTION(0x58, " CLI", cli, implied);

    /* CLV - Clear Overflow Flag. */
    SET_INSTRUCTION(0xB8, " CLV", clv, implied);

    /* CMP - Compare. */
    SET_INSTRUCTION(0xC9, " CMP", cmp, immediate);
    SET_INSTRUCTION(0xC5, " CMP", cmp, zero_page);
    SET_INSTRUCTION(0xD5, " CMP", cmp, zero_page_x);
    SET_INSTRUCTION(0xCD, " CMP", cmp, absolute);
    SET_INSTRUCTION(0xDD, " CMP", cmp, absolute_x);
    SET_INSTRUCTION(0xD9, " CMP", cmp, absolute_y);
    SET_INSTRUCTION(0xC1, " CMP", cmp, indirect_x);
    SET_INSTRUCTION(0xD1, " CMP", cmp, indirect_y);

    /* CPX - Compare X Register. */
    SET_INSTRUCTION(0xE0, " CPX", cpx, immediate);
    SET_INSTRUCTION(0xE4, " CPX", cpx, zero_page);
    SET_INSTRUCTION(0xEC, " CPX", cpx, absolute);

    /* CPY - Compare Y Register. */
    SET_INSTRUCTION(0xC0, " CPY", cpy, immediate);
    SET_INSTRUCTION(0xC4, " CPY", cpy, zero_page);
    SET_INSTRUCTION(0xCC, " CPY", cpy, absolute);

    /* DEC - Decrement Memory. */
    SET_INSTRUCTION(0xC6, " DEC", dec, zero_page);
    SET_INSTRUCTION(0xD6, " DEC", dec, zero_page_x);
    SET_INSTRUCTION(0xCE, " DEC", dec, absolute);
    SET_INSTRUCTION(0xDE, " DEC", dec, absolute_x_modify);

    /* DEX - Decrement X Register. */
    SET_INSTRUCTION(0xCA, " DEX", dex, implied);

    /* DEY - Decrement Y Register. */
    SET_INSTRUCTION(0x88, " DEY", dey, implied);

    /* DCP - Decrement and Compare. */
    SET_INSTRUCTION(0xC7, "*DCP", dcp, zero_page);
    SET_INSTRUCTION(0xD7, "*DCP", dcp, zero_page_x);
    SET_INSTRUCTION(0xCF, "*DCP", dcp, absolute);
    SET_INSTRUCTION(0xDF, "*DCP", dcp, absolute_x_modify);
    SET_INSTRUCTION(0xDB, "*DCP", dcp, absolute_y);
    SET_INSTRUCTION(0xC3, "*DCP", dcp, indirect_x);
    SET_INSTRUCTION(0xD3, "*DCP", dcp, indirect_y);

    /* EOR - Exclusive OR. */
    SET_INSTRUCTION(0x49, " EOR", eor, immediate);
    SET_INSTRUCTION(0x45, " EOR", eor, zero_page);
    SET_INSTRUCTION(0x55, " EOR", eor, zero_page_x);
    SET_INSTRUCTION(0x4D, " EOR", eor, absolute);
    SET_INSTRUCTION(0x5D, " EOR", eor, absolute_x);
    SET_INSTRUCTION(0x59, " EOR", eor, absolute_y);
    SET_INSTRUCTION(0x41, " EOR", eor, indirect_x);
    SET_INSTRUCTION(0x51, " EOR", eor, indirect_y);

    /* HLT - Halt. */
    SET_INSTRUCTION(0x02, "*HLT", hlt, immediate);
    SET_INSTRUCTION(0x12, "*HLT", hlt, immediate);
    SET_INSTRUCTION(0x22, "*HLT", hlt, immediate);
    SET_INSTRUCTION(0x32, "*HLT", hlt, immediate);
    SET_INSTRUCTION(0x42, "*HLT", hlt, immediate);
    SET_INSTRUCTION(0x52, "*HLT", hlt, immediate);
    SET_INSTRUCTION(0x62, "*HLT", hlt, immediate);
    SET_INSTRUCTION(0x72, "*HLT", hlt, immediate);
    SET_INSTRUCTION(0x92, "*HLT", hlt, immediate);
    SET_INSTRUCTION(0xB2, "*HLT", hlt, immediate);
    SET_INSTRUCTION(0xD2, "*HLT", hlt, immediate);
    SET_INSTRUCTION(0xF2, "*HLT", hlt, immediate);

    /* INC - Increment Memory. */
    SET_INSTRUCTION(0xE6, " INC", inc, zero_page);
    SET_INSTRUCTION(0xF6, " INC", inc, zero_page_x);
    SET_INSTRUCTION(0xEE, " INC", inc, absolute);
    SET_INSTRUCTION(0xFE, " INC", inc, absolute_x_modify);

    /* INX - Increment X Register. */
    SET_INSTRUCTION(0xE8, " INX", inx, implied);

    /* INY - Increment Y Register. */
    SET_INSTRUCTION(0xC8, " INY", iny, implied);

    /* ISB - Increment and subtract. */
    SET_INSTRUCTION(0xE7, "*ISB", isb, zero_page);
    SET_INSTRUCTION(0xF7, "*ISB", isb, zero_page_x);
    SET_INSTRUCTION(0xEF, "*ISB", isb, absolute);
    SET_INSTRUCTION(0xFF, "*ISB", isb, absolute_x_modify);
    SET_INSTRUCTION(0xFB, "*ISB", isb, absolute_y);
    SET_INSTRUCTION(0xE3, "*ISB", isb, indirect_x);
    SET_INSTRUCTION(0xF3, "*ISB", isb, indirect_y);

    /* JMP - Jump. */
    SET_INSTRUCTION(0x4C, " JMP", jmp_absolute, absolute_jump);
    SET_INSTRUCTION(0x6C, " JMP", jmp_indirect, indirect);

    /* JSR - Jump to Subroutine. */
    SET_INSTRUCTION(0x20, " JSR", jsr, absolute_jump);

    /* LAX - Load Accumulator and X. */
    SET_INSTRUCTION(0xAB, "*LAX", lax, immediate);
    SET_INSTRUCTION(0xA7, "*LAX", lax, zero_page);
    SET_INSTRUCTION(0xB7, "*LAX", lax, zero_page_y);
    SET_INSTRUCTION(0xAF, "*LAX", lax, absolute);
    SET_INSTRUCTION(0xBF, "*LAX", lax, absolute_y);
    SET_INSTRUCTION(0xA3, "*LAX", lax, indirect_x);
    SET_INSTRUCTION(0xB3, "*LAX", lax, indirect_y);

    /* LDA - Load Accumulator. */
    SET_INSTRUCTION(0xA9, " LDA", lda, immediate);
    SET_INSTRUCTION(0xA5, " LDA", lda, zero_page);
    SET_INSTRUCTION(0xB5, " LDA", lda, zero_page_x);
    SET_INSTRUCTION(0xAD, " LDA", lda, absolute);
    SET_INSTRUCTION(0xBD, " LDA", lda, absolute_x);
    SET_INSTRUCTION(0xB9, " LDA", lda, absolute_y);
    SET_INSTRUCTION(0xA1, " LDA", lda, indirect_x);
    SET_INSTRUCTION(0xB1, " LDA", lda, indirect_y);

    /* LDX - Load X Register. */
    SET_INSTRUCTION(0xA2, " LDX", ldx, immediate);
    SET_INSTRUCTION(0xA6, " LDX", ldx, zero_page);
    SET_INSTRUCTION(0xB6, " LDX", ldx, zero_page_y);
    SET_INSTRUCTION(0xAE, " LDX", ldx, absolute);
    SET_INSTRUCTION(0xBE, " LDX", ldx, absolute_y);

    /* LDY - Load Y Register. */
    SET_INSTRUCTION(0xA0, " LDY", ldy, immediate);
    SET_INSTRUCTION(0xA4, " LDY", ldy, zero_page);
    SET_INSTRUCTION(0xB4, " LDY", ldy, zero_page_x);
    SET_INSTRUCTION(0xAC, " LDY", ldy, absolute);
    SET_INSTRUCTION(0xBC, " LDY", ldy, absolute_x);

    /* LSR - Logical Shift Right. */
    SET_INSTRUCTION(0x4A, " LSR", lsr_a, accumulator);
    SET_INSTRUCTION(0x46, " LSR", lsr_m, zero_page);
    SET_INSTRUCTION(0x56, " LSR", lsr_m, zero_page_x);
    SET_INSTRUCTION(0x4E, " LSR", lsr_m, absolute);
    SET_INSTRUCTION(0x5E, " LSR", lsr_m, absolute_x_modify);

    /* NOP - No Operation. */
    SET_INSTRUCTION(0xEA, " NOP", nop, implied);
    SET_INSTRUCTION(0x1A, "*NOP", nop, implied);
    SET_INSTRUCTION(0x3A, "*NOP", nop, implied);
    SET_INSTRUCTION(0x5A, "*NOP", nop, implied);
    SET_INSTRUCTION(0x7A, "*NOP", nop, implied);
    SET_INSTRUCTION(0xDA, "*NOP", nop, implied);
    SET_INSTRUCTION(0xFA, "*NOP", nop, implied);
    SET_INSTRUCTION(0x80, "*NOP", nop, immediate);
    SET_INSTRUCTION(0x82, "*NOP", nop, immediate);
    SET_INSTRUCTION(0xC2, "*NOP", nop, immediate);
    SET_INSTRUCTION(0xE2, "*NOP", nop, immediate);
    SET_INSTRUCTION(0x89, "*NOP", nop, immediate);
    SET_INSTRUCTION(0x04, "*NOP", nop, zero_page);
    SET_INSTRUCTION(0x44, "*NOP", nop, zero_page);
    SET_INSTRUCTION(0x64, "*NOP", nop, zero_page);
    SET_INSTRUCTION(0x14, "*NOP", nop, zero_page_x);
    SET_INSTRUCTION(0x34, "*NOP", nop, zero_page_x);
    SET_INSTRUCTION(0x54, "*NOP", nop, zero_page_x);
    SET_INSTRUCTION(0x74, "*NOP", nop, zero_page_x);
    SET_INSTRUCTION(0xD4, "*NOP", nop, zero_page_x);
    SET_INSTRUCTION(0xF4, "*NOP", nop, zero_page_x);
    SET_INSTRUCTION(0x0C, "*NOP", nop, absolute);
    SET_INSTRUCTION(0x1C, "*NOP", nop, absolute_x);
    SET_INSTRUCTION(0x3C, "*NOP", nop, absolute_x);
    SET_INSTRUCTION(0x5C, "*NOP", nop, absolute_x);
    SET_INSTRUCTION(0x7C, "*NOP", nop, absolute_x);
    SET_INSTRUCTION(0xDC, "*NOP", nop, absolute_x);
    SET_INSTRUCTION(0xFC, "*NOP", nop, absolute_x);

    /* ORA - Logical Inclusive OR. */
    SET_INSTRUCTION(0x09, " ORA", ora, immediate);
    SET_INSTRUCTION(0x05, " ORA", ora, zero_page);
    SET_INSTRUCTION(0x15, " ORA", ora, zero_page_x);
    SET_INSTRUCTION(0x0D, " ORA", ora, absolute);
    SET_INSTRUCTION(0x1D, " ORA", ora, absolute_x);
    SET_INSTRUCTION(0x19, " ORA", ora, absolute_y);
    SET_INSTRUCTION(0x01, " ORA", ora, indirect_x);
    SET_INSTRUCTION(0x11, " ORA", ora, indirect_y);

    /* PHA - Push Accumulator. */
    SET_INSTRUCTION(0x48, " PHA", pha, implied);

    /* PHP - Push Processor Status. */
    SET_INSTRUCTION(0x08, " PHP", php, implied);

    /* PLA - Pull Accumulator. */
    SET_INSTRUCTION(0x68, " PLA", pla, implied);

    /* PLP - Pull Processor Status. */
    SET_INSTRUCTION(0x28, " PLP", plp, implied);

    /* RLA - Rotate Left and AND. */
    SET_INSTRUCTION(0x27, "*RLA", rla, zero_page);
    SET_INSTRUCTION(0x37, "*RLA", rla, zero_page_x);
    SET_INSTRUCTION(0x2F, "*RLA", rla, absolute);
    SET_INSTRUCTION(0x3F, "*RLA", rla, absolute_x_modify);
    SET_INSTRUCTION(0x3B, "*RLA", rla, absolute_y);
    SET_INSTRUCTION(0x23, "*RLA", rla, indirect_x);
    SET_INSTRUCTION(0x33, "*RLA", rla, indirect_y);

    /* ROL - Rotate Left. */
    SET_INSTRUCTION(0x2A, " ROL", rol_a, accumulator);
    SET_INSTRUCTION(0x26, " ROL", rol_m, zero_page);
    SET_INSTRUCTION(0x36, " ROL", rol_m, zero_page_x);
    SET_INSTRUCTION(0x2E, " ROL", rol_m, absolute);
    SET_INSTRUCTION(0x3E, " ROL", rol_m, absolute_x_modify);

    /* ROR - Rotate Right. */
    SET_INSTRUCTION(0x6A, " ROR", ror_a, accumulator);
    SET_INSTRUCTION(0x66, " ROR", ror_m, zero_page);
    SET_INSTRUCTION(0x76, " ROR", ror_m, zero_page_x);
    SET_INSTRUCTION(0x6E, " ROR", ror_m, absolute);
    SET_INSTRUCTION(0x7E, " ROR", ror_m, absolute_x_modify);

    /* RRA - Rotate Right and Add. */
    SET_INSTRUCTION(0x67, "*RRA", rra, zero_page);
    SET_INSTRUCTION(0x77, "*RRA", rra, zero_page_x);
    SET_INSTRUCTION(0x6F, "*RRA", rra, absolute);
    SET_INSTRUCTION(0x7F, "*RRA", rra, absolute_x_modify);
    SET_INSTRUCTION(0x7B, "*RRA", rra, absolute_y);
    SET_INSTRUCTION(0x63, "*RRA", rra, indirect_x);
    SET_INSTRUCTION(0x73, "*RRA", rra, indirect_y);

    /* RTI - Return from Interrupt. */
    SET_INSTRUCTION(0x40, " RTI", rti, implied);

    /* RTS - Return from Subroutine. */
    SET_INSTRUCTION(0x60, " RTS", rts, implied);

    /* SAX - Store Accumulator AND X. */
    SET_INSTRUCTION(0x87, "*SAX", sax, zero_page_write);
    SET_INSTRUCTION(0x97, "*SAX", sax, zero_page_y_write);
    SET_INSTRUCTION(0x8F, "*SAX", sax, absolute_write);
    SET_INSTRUCTION(0x83, "*SAX", sax, indirect_x_write); 

    /* SBC - Subtract with Carry. */
    SET_INSTRUCTION(0xE9, " SBC", sbc, immediate);
    SET_INSTRUCTION(0xEB, "*SBC", sbc, immediate);
    SET_INSTRUCTION(0xE5, " SBC", sbc, zero_page);
    SET_INSTRUCTION(0xF5, " SBC", sbc, zero_page_x);
    SET_INSTRUCTION(0xED, " SBC", sbc, absolute);
    SET_INSTRUCTION(0xFD, " SBC", sbc, absolute_x);
    SET_INSTRUCTION(0xF9, " SBC", sbc, absolute_y);
    SET_INSTRUCTION(0xE1, " SBC", sbc, indirect_x);
    SET_INSTRUCTION(0xF1, " SBC", sbc, indirect_y);

    /* SEC - Set Carry Flag. */
    SET_INSTRUCTION(0x38, " SEC", sec, implied);

    /* SEC - Set Decimal Flag. */
    SET_INSTRUCTION(0xF8, " SED", sed, implied);

    /* SEI - Set Interrupt Disable. */
    SET_INSTRUCTION(0x78, " SEI", sei, implied);

    /* SLO - Shift Left and Inclusive OR. */
    SET_INSTRUCTION(0x07, "*SLO", slo, zero_page);
    SET_INSTRUCTION(0x17, "*SLO", slo, zero_page_x);
    SET_INSTRUCTION(0x0F, "*SLO", slo, absolute);
    SET_INSTRUCTION(0x1F, "*SLO", slo, absolute_x_modify);
    SET_INSTRUCTION(0x1B, "*SLO", slo, absolute_y);
    SET_INSTRUCTION(0x03, "*SLO", slo, indirect_x);
    SET_INSTRUCTION(0x13, "*SLO", slo, indirect_y);

    /* SRE - Shift Right and Exclusive OR. */
    SET_INSTRUCTION(0x47, "*SRE", sre, zero_page);
    SET_INSTRUCTION(0x57, "*SRE", sre, zero_page_x);
    SET_INSTRUCTION(0x4F, "*SRE", sre, absolute);
    SET_INSTRUCTION(0x5F, "*SRE", sre, absolute_x_modify);
    SET_INSTRUCTION(0x5B, "*SRE", sre, absolute_y);
    SET_INSTRUCTION(0x43, "*SRE", sre, indirect_x);
    SET_INSTRUCTION(0x53, "*SRE", sre, indirect_y);

    /* STA - Store Accumulator. */
    SET_INSTRUCTION(0x85, " STA", sta, zero_page_write);
    SET_INSTRUCTION(0x95, " STA", sta, zero_page_x_write);
    SET_INSTRUCTION(0x8D, " STA", sta, absolute_write);
    SET_INSTRUCTION(0x9D, " STA", sta, absolute_x_write);
    SET_INSTRUCTION(0x99, " STA", sta, absolute_y_write);
    SET_INSTRUCTION(0x81, " STA", sta, indirect_x_write);
    SET_INSTRUCTION(0x91, " STA", sta, indirect_y);

    /* STX - Store X Register. */
    SET_INSTRUCTION(0x86, " STX", stx, zero_page_write);
    SET_INSTRUCTION(0x96, " STX", stx, zero_page_y_write);
    SET_INSTRUCTION(0x8E, " STX", stx, absolute_write);

    /* STY - Store Y Register. */
    SET_INSTRUCTION(0x84, " STY", sty, zero_page_write);
    SET_INSTRUCTION(0x94, " STY", sty, zero_page_x_write);
    SET_INSTRUCTION(0x8C, " STY", sty, absolute_write);

    /* TAX - Transfer Accumulator to X. */
    SET_INSTRUCTION(0xAA, " TAX", tax, implied);

    /* TAY - Transfer Accumulator to Y. */
    SET_INSTRUCTION(0xA8, " TAY", tay, implied);

    /* TSX - Transfer Stack Pointer to X. */
    SET_INSTRUCTION(0xBA, " TSX", tsx, implied);

    /* TSA - Transfer X to Accumulator. */
    SET_INSTRUCTION(0x8A, " TXA", txa, implied);

    /* TXS - Transfer X to Stack Pointer. */
    SET_INSTRUCTION(0x9A, " TXS", txs, implied);

    /* TYA - Transfer Y to Accumulator. */
    SET_INSTRUCTION(0x98, " TYA", tya, implied);

    /* XAA - Transfer X to Accumulator and AND. */
    SET_INSTRUCTION(0x8B, "*XAA", xaa, implied);
}

/* -----------------------------------------------------------------
 * CPU iterface.
 * -------------------------------------------------------------- */

void cpu_set_nmi(void) {
    nmi = true;
}

void cpu_reset(void) {
    cpu.S -= 3;
    flg_set_I();
    /* ... */
}

void cpu_init(void) {
    /* Initialize instruction table. */
    if (!initialized_table) {
        init_instruction_table();
    }
    initialized_table = true;

    /* Initialize CPU status. */
    cpu = (CPU) { 0x0000, 0xFD, 0x00, 0x00, 0x00 };
    cpu.PC = mem_read_16(RESET_VECTOR);

    /* Clear RAM. */
    for (int i = 0; i < RAM_SIZE; i++) {
        cpu.ram[i] = 0x00;
    }

    /* Clear all flags; IRQ disabled. */
    flg_reset();
    flg_set_I();
}

void cpu_execute(void) {
    if (nmi) {
        cpu_interrupt(NMI_VECTOR);
        nmi = false;
    }
    else {
        #ifdef CPU_LOGGING
        cpu_log_operation();
        #endif

        opcode = cpu_fetch();               /* Fetch opcode. */
        (*cpu_addressing_table[opcode])();  /* Fetch arguments. */
        (*cpu_instruction_table[opcode])(); /* Execute operation. */
    }
}

inline unsigned long long cpu_get_ticks(void) {
    return cycles;
}

inline void cpu_suspend(int num_cycles) {
    cycles += num_cycles;
}

inline byte cpu_ram_read(word address) {
    return cpu.ram[address];
}

inline void cpu_ram_write(word address, byte data) {
    cpu.ram[address] = data;
}
