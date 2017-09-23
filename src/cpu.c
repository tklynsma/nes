#define CPU_LOGGING

#include <stdio.h>
#include "../include/common.h"
#include "../include/cpu.h"
#include "../include/cpu_flags.h"
#include "../include/cpu_internal.h"
#include "../include/log.h"
#include "../include/memory.h"

#ifdef CPU_LOGGING
#include "../include/ppu_internal.h"
#endif

/* -----------------------------------------------------------------
 * CPU variables.
 * -------------------------------------------------------------- */

/* CPU instruction tables. */
typedef void (*Function)(void);
static Function cpu_instruction_table[256];
static Function cpu_addressing_table[256];
static int cpu_cycles_table[256];
static char *cpu_names_table[256];

CPU cpu;                    /* CPU status. */

static bool nmi = false;    /* NMI interrupt. */

static byte opcode;         /* Current opcode being executed. */
static byte operand;        /* Operand (8 bit) of the instruction. */
static word address;        /* Operand (16 bit) of the instruction. */

static int wait_cycles;     /* Number of cycles to wait till the next operation. */
static int extra_cycles;    /* Number of additional cycles to be added. */
static unsigned long long cycles; /* Total number of cycles run so far. */

static bool initialized_table = false;

/* -----------------------------------------------------------------
 * Stack operations.
 * -------------------------------------------------------------- */

static inline void push(byte data) {
    cpu.ram[0x100 | cpu.S--] = data;
}

static inline void push_address(word address) {
    cpu.ram[0x100 | cpu.S--] = address >> 8;
    cpu.ram[0x100 | cpu.S--] = address & 0xFF;
}

static inline byte pop(void) {
    return cpu.ram[0x100 | ++cpu.S];
}

static inline word pop_address(void) {
    byte lo = cpu.ram[0x100 | ++cpu.S];
    byte hi = cpu.ram[0x100 | ++cpu.S];
    return (hi << 8) | lo;
}

/* -----------------------------------------------------------------
 * Handle interrupt.
 * -------------------------------------------------------------- */

static inline void interrupt(word vector) {
    LOG_CPU(32, "---INTERRUPT---");

    push_address(cpu.PC + 1);
    push(flg_get_status(false));
    cpu.PC = mem_read_16(vector);
    flg_set_I();
    wait_cycles = 7;
}

/* -----------------------------------------------------------------
 * CPU addressing modes.
 * -------------------------------------------------------------- */

static inline bool is_diff_page(word address1, word address2) {
    return (address1 & 0xFF00) != (address2 & 0xFF00);
}

static inline void absolute(void) {
    address = mem_read_16(cpu.PC + 1);
    operand = mem_read(address);
    cpu.PC += 3;

    LOG_CPU(6, "%02X %02X", address & 0xFF, address >> 8);
    if (opcode == 0x20 || opcode == 0x4C) /* JMP or JSR absolute. */
         LOG_CPU(33, "%s $%04X", cpu_names_table[opcode], address);
    else if (address >= 0x2000 && address < 0x4020)
         LOG_CPU(33, "%s $%04X = %02X", cpu_names_table[opcode], address, 0xFF);
    else LOG_CPU(33, "%s $%04X = %02X", cpu_names_table[opcode], address, operand);
}

static inline void absolute_x(void) {
    address = mem_read_16(cpu.PC + 1) + cpu.X;
    operand = mem_read(address);
    extra_cycles = is_diff_page(address, address - cpu.X);
    cpu.PC += 3;

    LOG_CPU(6, "%02X %02X", (address - cpu.X) & 0xFF, (address - cpu.X) >> 8);
    LOG_CPU(33, "%s $%04X,X @ %04X = %02X", cpu_names_table[opcode], address - cpu.X,
        address, operand);
}

static inline void absolute_y(void) {
    address = mem_read_16(cpu.PC + 1) + cpu.Y;
    operand = mem_read(address);
    extra_cycles = is_diff_page(address, address - cpu.Y);
    cpu.PC += 3;

    LOG_CPU(6, "%02X %02X", (address - cpu.Y) & 0xFF, (address - cpu.Y) >> 8);
    LOG_CPU(33, "%s $%04X,Y @ %04X = %02X", cpu_names_table[opcode], address - cpu.Y, 
        address, operand);
}

static inline void accumulator(void) {
    cpu.PC += 1;

    LOG_CPU(6, "");
    LOG_CPU(33, "%s A", cpu_names_table[opcode]);
}

static inline void immediate(void) {
    operand = mem_read(cpu.PC + 1);
    cpu.PC += 2;

    LOG_CPU(6, "%02X", operand);
    LOG_CPU(33, "%s #$%02X", cpu_names_table[opcode], operand);
}

static inline void implied(void) {
    cpu.PC += 1;

    LOG_CPU(6, "");
    LOG_CPU(33, "%s", cpu_names_table[opcode]);
}

static inline void indirect(void) {
    word address_ = mem_read_16(cpu.PC + 1);
    if (address_ & 0xFF == 0xFF) {
        byte lo = mem_read(address_);
        byte hi = mem_read(address_ - 0xFF);
        address = (hi << 8) | lo;
    }
    else {
        address = mem_read_16(address_);
    }

    LOG_CPU(6, "%02X %02X", address_ & 0xFF, address_ >> 8);
    LOG_CPU(33, "%s ($%04X) = %04X", cpu_names_table[opcode], address_, address);
}

static inline void indirect_x(void) {
    byte address_ = mem_read(cpu.PC + 1);
    byte lo = cpu.ram[(address_ + cpu.X) & 0xFF];
    byte hi = cpu.ram[(address_ + cpu.X + 1) & 0xFF];
    address = (hi << 8) | lo;
    operand = mem_read(address);
    cpu.PC += 2;

    LOG_CPU(6, "%02X", address_);
    LOG_CPU(33, "%s ($%02X,X) @ %02X = %04X = %02X", cpu_names_table[opcode],
        address_, (address_ + cpu.X) & 0xFF, address, operand);
}

static inline void indirect_y(void) {
    byte address_ = mem_read(cpu.PC + 1);
    byte lo = cpu.ram[address_];
    byte hi = cpu.ram[(address_ + 1) & 0xFF];
    address = ((hi << 8) | lo) + cpu.Y;
    operand = mem_read(address);
    extra_cycles = is_diff_page(address, address - cpu.Y);
    cpu.PC += 2;

    LOG_CPU(6, "%02X", address_);
    LOG_CPU(33, "%s ($%02X),Y = %04X @ %04X = %02X", cpu_names_table[opcode],
        address_, (hi << 8) | lo, address, operand);
}

static inline void relative(void) {
    operand = mem_read(cpu.PC + 1);
    cpu.PC += 2;

    LOG_CPU(6, "%02X", operand);
    LOG_CPU(33, "%s $%04X", cpu_names_table[opcode], cpu.PC + (int8_t) operand);
}

static inline void zero_page(void) {
    address = mem_read(cpu.PC + 1);
    operand = mem_read(address);
    cpu.PC += 2;

    LOG_CPU(6, "%02X", address);
    LOG_CPU(33, "%s $%02X = %02X", cpu_names_table[opcode], address, operand);
}

static inline void zero_page_x(void) {
    byte address_ = mem_read(cpu.PC + 1);
    address = (address_ + cpu.X) & 0xFF;
    operand = mem_read(address);
    cpu.PC += 2;

    LOG_CPU(6, "%02X", address_);
    LOG_CPU(33, "%s $%02X,X @ %02X = %02X", cpu_names_table[opcode], address_,
        address, operand);
}

static inline void zero_page_y(void) {
    byte address_ = mem_read(cpu.PC + 1);
    address = (address_ + cpu.Y) & 0xFF;
    operand = mem_read(address);
    cpu.PC += 2;

    LOG_CPU(6, "%02X", address_);
    LOG_CPU(33, "%s $%02X,Y @ %02X = %02X", cpu_names_table[opcode], address_,
        address, operand);
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

        extra_cycles++;
        if (is_diff_page(address, cpu.PC)) {
            extra_cycles++;  /* Page crossed. */
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
    push_address(cpu.PC + 1);
    push(flg_get_status(true));
    cpu.PC = mem_read_16(IRQ_VECTOR);
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
    flg_update_ZN(cpu.A ^= operand);
}

/* INC - Increment Memory. */
static inline void inc(void) {
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
    push_address(cpu.PC - 1);
    cpu.PC = address;
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
    mem_write(address, operand >>= 1);
    flg_update_ZN(operand);
    extra_cycles = 0;
}

/* NOP - No Operation. */
static inline void nop(void) {}

/* ORA - Logical Inclusive OR. */
static inline void ora(void) {
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
    flg_update_ZN(cpu.A = pop());
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
    flg_update_C (operand, ROL);
    operand = (operand << 1) | flg_is_C();
    mem_write(address, operand);
    flg_update_ZN(operand);
    extra_cycles = 0;
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
    operand = (operand >> 1) | (carry << 7);
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
    cpu.PC = pop_address() + 1;
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
    mem_write(address, --operand);
    flg_update_C (cpu.A - operand, CMP);
    flg_update_ZN(cpu.A - operand);
    extra_cycles = 0;
}

/* LAX - Load Accumulator and X. */
static inline void lax(void) {
    flg_update_ZN(cpu.A = cpu.X = operand);
}

/* HLT - Halt. */
static inline void hlt(void) {
    LOG_ERROR("CPU halted, opcode: %02X.", opcode);
}

/* ISB - Increment and Subtract. */
static inline void isb(void) {
    mem_write(address, ++operand);
    extra_cycles = 0;

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
    operand = (operand << 1) | carry;
    mem_write(address, operand);
    flg_update_ZN(cpu.A &= operand);
    extra_cycles = 0;
}

/* RRA - Rotate Right and Add. */
static inline void rra(void) {
    bool carry = flg_is_C();
    flg_update_C (operand, ROR);
    operand = (operand >> 1) | (carry << 7);
    mem_write(address, operand);

    int result = cpu.A + operand + flg_is_C();
    flg_update_ZN(result);
    flg_update_C (result, ADC);
    flg_update_V (result, cpu.A, operand);
    cpu.A = result & 0xFF;
    extra_cycles = 0;
}

/* SAX - Store Accumulator AND X. */
static inline void sax(void) {
    mem_write(address, cpu.A & cpu.X);
}

/* SLO - Shift Left and Inclusive OR. */
static inline void slo(void) {
    flg_update_C (operand, ROL);
    mem_write(address, operand <<= 1);
    flg_update_ZN(cpu.A |= operand);
    extra_cycles = 0;
}

/* SRE - Shift Right and Exclusive OR. */
static inline void sre(void) {
    flg_update_C (operand, ROR);
    mem_write(address, operand >>= 1);
    flg_update_ZN(cpu.A ^= operand);
    extra_cycles = 0;
}

/* XAA - Transfer X to Accumulator and AND. */
static inline void xaa(void) {
    cpu.A = cpu.X & operand;
    flg_update_ZN(cpu.A);
}

/* --------------------------------------------------------------------
 * CPU operation tables.
 * ----------------------------------------------------------------- */

static inline void set_instruction(byte opcode, int cycles,
        char* name, Function operation, Function mode) {
    cpu_instruction_table[opcode] = operation;
    cpu_addressing_table[opcode] = mode;
    cpu_cycles_table[opcode] = cycles;
    cpu_names_table[opcode] = name;
}

static inline void init_instruction_table(void) {
    /* Reset CPU tables. */
    for (int opcode = 0; opcode < 256; opcode++) {
        set_instruction(opcode, 2, "****", invalid, implied);
    }

    /* ADC - Add with Carry. */
    set_instruction(0x69, 2, " ADC", adc, immediate);
    set_instruction(0x65, 3, " ADC", adc, zero_page);
    set_instruction(0x75, 4, " ADC", adc, zero_page_x);
    set_instruction(0x6D, 4, " ADC", adc, absolute);
    set_instruction(0x7D, 4, " ADC", adc, absolute_x);
    set_instruction(0x79, 4, " ADC", adc, absolute_y);
    set_instruction(0x61, 6, " ADC", adc, indirect_x);
    set_instruction(0x71, 5, " ADC", adc, indirect_y);

    /* ALR - AND and Shift Right. */
    set_instruction(0x4B, 2, "*ALR", alr, immediate);

    /* ANC - AND with Carry. */
    set_instruction(0x0B, 2, "*ANC", anc, immediate);
    set_instruction(0x2B, 2, "*ANC", anc, immediate);

    /* AND - Logical AND. */
    set_instruction(0x29, 2, " AND", and, immediate);
    set_instruction(0x25, 3, " AND", and, zero_page);
    set_instruction(0x35, 4, " AND", and, zero_page_x);
    set_instruction(0x2D, 4, " AND", and, absolute);
    set_instruction(0x3D, 4, " AND", and, absolute_x);
    set_instruction(0x39, 4, " AND", and, absolute_y);
    set_instruction(0x21, 6, " AND", and, indirect_x);
    set_instruction(0x31, 5, " AND", and, indirect_y);

    /* ARR - AND and Rotate Right. */
    set_instruction(0x6B, 2, "*ARR", arr, immediate);

    /* ASL - Arithmetic Shift Left. */
    set_instruction(0x0A, 2, " ASL", asl_a, accumulator);
    set_instruction(0x06, 5, " ASL", asl_m, zero_page);
    set_instruction(0x16, 6, " ASL", asl_m, zero_page_x);
    set_instruction(0x0E, 6, " ASL", asl_m, absolute);
    set_instruction(0x1E, 7, " ASL", asl_m, absolute_x);

    /* AXS - AND X with Accumulator and Subtract. */
    set_instruction(0xCB, 2, "*AXS", axs, immediate);

    /* BCC - Branch if Carry Clear. */
    set_instruction(0x90, 2, " BCC", bcc, relative);

    /* BCS - Branch if Carry Set. */
    set_instruction(0xB0, 2, " BCS", bcs, relative);

    /* BEQ - Branch if Equal. */
    set_instruction(0xF0, 2, " BEQ", beq, relative);

    /* BIT - Bit Test. */
    set_instruction(0x24, 3, " BIT", bit, zero_page);
    set_instruction(0x2C, 4, " BIT", bit, absolute);

    /* BMI - Branch if Minus. */
    set_instruction(0x30, 2, " BMI", bmi, relative);

    /* BNE - Branch if Not Equal. */
    set_instruction(0xD0, 2, " BNE", bne, relative);

    /* BPL - Branch if Positive. */
    set_instruction(0x10, 2, " BPL", bpl, relative);

    /* BRK - Force Interrupt. */
    set_instruction(0x00, 7, " BRK", brk, implied);

    /* BVC - Branch if Overflow Clear. */
    set_instruction(0x50, 2, " BVC", bvc, relative);

    /* BVS - Branch if Overflow Set. */
    set_instruction(0x70, 2, " BVS", bvs, relative);

    /* CLC - Clear Carry Flag. */
    set_instruction(0x18, 2, " CLC", clc, implied);

    /* CLD - Clear Decimal Mode. */
    set_instruction(0xD8, 2, " CLD", cld, implied);

    /* CLI - Clear Interrupt Disable. */
    set_instruction(0x58, 2, " CLI", cli, implied);

    /* CLV - Clear Overflow Flag. */
    set_instruction(0xB8, 2, " CLV", clv, implied);

    /* CMP - Compare. */
    set_instruction(0xC9, 2, " CMP", cmp, immediate);
    set_instruction(0xC5, 3, " CMP", cmp, zero_page);
    set_instruction(0xD5, 4, " CMP", cmp, zero_page_x);
    set_instruction(0xCD, 4, " CMP", cmp, absolute);
    set_instruction(0xDD, 4, " CMP", cmp, absolute_x);
    set_instruction(0xD9, 4, " CMP", cmp, absolute_y);
    set_instruction(0xC1, 6, " CMP", cmp, indirect_x);
    set_instruction(0xD1, 5, " CMP", cmp, indirect_y);

    /* CPX - Compare X Register. */
    set_instruction(0xE0, 2, " CPX", cpx, immediate);
    set_instruction(0xE4, 3, " CPX", cpx, zero_page);
    set_instruction(0xEC, 4, " CPX", cpx, absolute);

    /* CPY - Compare Y Register. */
    set_instruction(0xC0, 2, " CPY", cpy, immediate);
    set_instruction(0xC4, 3, " CPY", cpy, zero_page);
    set_instruction(0xCC, 4, " CPY", cpy, absolute);

    /* DEC - Decrement Memory. */
    set_instruction(0xC6, 5, " DEC", dec, zero_page);
    set_instruction(0xD6, 6, " DEC", dec, zero_page_x);
    set_instruction(0xCE, 6, " DEC", dec, absolute);
    set_instruction(0xDE, 7, " DEC", dec, absolute_x);

    /* DEX - Decrement X Register. */
    set_instruction(0xCA, 2, " DEX", dex, implied);

    /* DEY - Decrement Y Register. */
    set_instruction(0x88, 2, " DEY", dey, implied);

    /* DCP - Decrement and Compare. */
    set_instruction(0xC7, 5, "*DCP", dcp, zero_page);
    set_instruction(0xD7, 6, "*DCP", dcp, zero_page_x);
    set_instruction(0xCF, 6, "*DCP", dcp, absolute);
    set_instruction(0xDF, 7, "*DCP", dcp, absolute_x);
    set_instruction(0xDB, 7, "*DCP", dcp, absolute_y);
    set_instruction(0xC3, 8, "*DCP", dcp, indirect_x);
    set_instruction(0xD3, 8, "*DCP", dcp, indirect_y);

    /* EOR - Exclusive OR. */
    set_instruction(0x49, 2, " EOR", eor, immediate);
    set_instruction(0x45, 3, " EOR", eor, zero_page);
    set_instruction(0x55, 4, " EOR", eor, zero_page_x);
    set_instruction(0x4D, 4, " EOR", eor, absolute);
    set_instruction(0x5D, 4, " EOR", eor, absolute_x);
    set_instruction(0x59, 4, " EOR", eor, absolute_y);
    set_instruction(0x41, 6, " EOR", eor, indirect_x);
    set_instruction(0x51, 5, " EOR", eor, indirect_y);

    /* HLT - Halt. */
    set_instruction(0x02, 0, "*HLT", hlt, immediate);
    set_instruction(0x12, 0, "*HLT", hlt, immediate);
    set_instruction(0x22, 0, "*HLT", hlt, immediate);
    set_instruction(0x32, 0, "*HLT", hlt, immediate);
    set_instruction(0x42, 0, "*HLT", hlt, immediate);
    set_instruction(0x52, 0, "*HLT", hlt, immediate);
    set_instruction(0x62, 0, "*HLT", hlt, immediate);
    set_instruction(0x72, 0, "*HLT", hlt, immediate);
    set_instruction(0x92, 0, "*HLT", hlt, immediate);
    set_instruction(0xB2, 0, "*HLT", hlt, immediate);
    set_instruction(0xD2, 0, "*HLT", hlt, immediate);
    set_instruction(0xF2, 0, "*HLT", hlt, immediate);

    /* INC - Increment Memory. */
    set_instruction(0xE6, 5, " INC", inc, zero_page);
    set_instruction(0xF6, 6, " INC", inc, zero_page_x);
    set_instruction(0xEE, 6, " INC", inc, absolute);
    set_instruction(0xFE, 7, " INC", inc, absolute_x);

    /* INX - Increment X Register. */
    set_instruction(0xE8, 2, " INX", inx, implied);

    /* INY - Increment Y Register. */
    set_instruction(0xC8, 2, " INY", iny, implied);

    /* ISB - Increment and subtract. */
    set_instruction(0xE7, 5, "*ISB", isb, zero_page);
    set_instruction(0xF7, 6, "*ISB", isb, zero_page_x);
    set_instruction(0xEF, 6, "*ISB", isb, absolute);
    set_instruction(0xFF, 7, "*ISB", isb, absolute_x);
    set_instruction(0xFB, 7, "*ISB", isb, absolute_y);
    set_instruction(0xE3, 8, "*ISB", isb, indirect_x);
    set_instruction(0xF3, 8, "*ISB", isb, indirect_y);

    /* JMP - Jump. */
    set_instruction(0x4C, 3, " JMP", jmp, absolute);
    set_instruction(0x6C, 5, " JMP", jmp, indirect);

    /* JSR - Jump to Subroutine. */
    set_instruction(0x20, 6, " JSR", jsr, absolute);

    /* LAX - Load Accumulator and X. */
    set_instruction(0xAB, 2, "*LAX", lax, immediate);
    set_instruction(0xA7, 3, "*LAX", lax, zero_page);
    set_instruction(0xB7, 4, "*LAX", lax, zero_page_y);
    set_instruction(0xAF, 4, "*LAX", lax, absolute);
    set_instruction(0xBF, 4, "*LAX", lax, absolute_y);
    set_instruction(0xA3, 6, "*LAX", lax, indirect_x);
    set_instruction(0xB3, 5, "*LAX", lax, indirect_y);

    /* LDA - Load Accumulator. */
    set_instruction(0xA9, 2, " LDA", lda, immediate);
    set_instruction(0xA5, 3, " LDA", lda, zero_page);
    set_instruction(0xB5, 4, " LDA", lda, zero_page_x);
    set_instruction(0xAD, 4, " LDA", lda, absolute);
    set_instruction(0xBD, 4, " LDA", lda, absolute_x);
    set_instruction(0xB9, 4, " LDA", lda, absolute_y);
    set_instruction(0xA1, 6, " LDA", lda, indirect_x);
    set_instruction(0xB1, 5, " LDA", lda, indirect_y);

    /* LDX - Load X Register. */
    set_instruction(0xA2, 2, " LDX", ldx, immediate);
    set_instruction(0xA6, 3, " LDX", ldx, zero_page);
    set_instruction(0xB6, 4, " LDX", ldx, zero_page_y);
    set_instruction(0xAE, 4, " LDX", ldx, absolute);
    set_instruction(0xBE, 4, " LDX", ldx, absolute_y);

    /* LDY - Load Y Register. */
    set_instruction(0xA0, 2, " LDY", ldy, immediate);
    set_instruction(0xA4, 3, " LDY", ldy, zero_page);
    set_instruction(0xB4, 4, " LDY", ldy, zero_page_x);
    set_instruction(0xAC, 4, " LDY", ldy, absolute);
    set_instruction(0xBC, 4, " LDY", ldy, absolute_x);

    /* LSR - Logical Shift Right. */
    set_instruction(0x4A, 2, " LSR", lsr_a, accumulator);
    set_instruction(0x46, 5, " LSR", lsr_m, zero_page);
    set_instruction(0x56, 6, " LSR", lsr_m, zero_page_x);
    set_instruction(0x4E, 6, " LSR", lsr_m, absolute);
    set_instruction(0x5E, 7, " LSR", lsr_m, absolute_x);

    /* NOP - No Operation. */
    set_instruction(0xEA, 2, " NOP", nop, implied);
    set_instruction(0x1A, 2, "*NOP", nop, implied);
    set_instruction(0x3A, 2, "*NOP", nop, implied);
    set_instruction(0x5A, 2, "*NOP", nop, implied);
    set_instruction(0x7A, 2, "*NOP", nop, implied);
    set_instruction(0xDA, 2, "*NOP", nop, implied);
    set_instruction(0xFA, 2, "*NOP", nop, implied);
    set_instruction(0x80, 2, "*NOP", nop, immediate);
    set_instruction(0x82, 2, "*NOP", nop, immediate);
    set_instruction(0xC2, 2, "*NOP", nop, immediate);
    set_instruction(0xE2, 2, "*NOP", nop, immediate);
    set_instruction(0x89, 2, "*NOP", nop, immediate);
    set_instruction(0x04, 3, "*NOP", nop, zero_page);
    set_instruction(0x44, 3, "*NOP", nop, zero_page);
    set_instruction(0x64, 3, "*NOP", nop, zero_page);
    set_instruction(0x14, 4, "*NOP", nop, zero_page_x);
    set_instruction(0x34, 4, "*NOP", nop, zero_page_x);
    set_instruction(0x54, 4, "*NOP", nop, zero_page_x);
    set_instruction(0x74, 4, "*NOP", nop, zero_page_x);
    set_instruction(0xD4, 4, "*NOP", nop, zero_page_x);
    set_instruction(0xF4, 4, "*NOP", nop, zero_page_x);
    set_instruction(0x0C, 4, "*NOP", nop, absolute);
    set_instruction(0x1C, 4, "*NOP", nop, absolute_x);
    set_instruction(0x3C, 4, "*NOP", nop, absolute_x);
    set_instruction(0x5C, 4, "*NOP", nop, absolute_x);
    set_instruction(0x7C, 4, "*NOP", nop, absolute_x);
    set_instruction(0xDC, 4, "*NOP", nop, absolute_x);
    set_instruction(0xFC, 4, "*NOP", nop, absolute_x);

    /* ORA - Logical Inclusive OR. */
    set_instruction(0x09, 2, " ORA", ora, immediate);
    set_instruction(0x05, 3, " ORA", ora, zero_page);
    set_instruction(0x15, 4, " ORA", ora, zero_page_x);
    set_instruction(0x0D, 4, " ORA", ora, absolute);
    set_instruction(0x1D, 4, " ORA", ora, absolute_x);
    set_instruction(0x19, 4, " ORA", ora, absolute_y);
    set_instruction(0x01, 6, " ORA", ora, indirect_x);
    set_instruction(0x11, 5, " ORA", ora, indirect_y);

    /* PHA - Push Accumulator. */
    set_instruction(0x48, 3, " PHA", pha, implied);

    /* PHP - Push Processor Status. */
    set_instruction(0x08, 3, " PHP", php, implied);

    /* PLA - Pull Accumulator. */
    set_instruction(0x68, 4, " PLA", pla, implied);

    /* PLP - Pull Processor Status. */
    set_instruction(0x28, 4, " PLP", plp, implied);

    /* RLA - Rotate Left and AND. */
    set_instruction(0x27, 5, "*RLA", rla, zero_page);
    set_instruction(0x37, 6, "*RLA", rla, zero_page_x);
    set_instruction(0x2F, 6, "*RLA", rla, absolute);
    set_instruction(0x3F, 7, "*RLA", rla, absolute_x);
    set_instruction(0x3B, 7, "*RLA", rla, absolute_y);
    set_instruction(0x23, 8, "*RLA", rla, indirect_x);
    set_instruction(0x33, 8, "*RLA", rla, indirect_y);

    /* ROL - Rotate Left. */
    set_instruction(0x2A, 2, " ROL", rol_a, accumulator);
    set_instruction(0x26, 5, " ROL", rol_m, zero_page);
    set_instruction(0x36, 6, " ROL", rol_m, zero_page_x);
    set_instruction(0x2E, 6, " ROL", rol_m, absolute);
    set_instruction(0x3E, 7, " ROL", rol_m, absolute_x);

    /* ROR - Rotate Right. */
    set_instruction(0x6A, 2, " ROR", ror_a, accumulator);
    set_instruction(0x66, 5, " ROR", ror_m, zero_page);
    set_instruction(0x76, 6, " ROR", ror_m, zero_page_x);
    set_instruction(0x6E, 6, " ROR", ror_m, absolute);
    set_instruction(0x7E, 7, " ROR", ror_m, absolute_x);

    /* RRA - Rotate Right and Add. */
    set_instruction(0x67, 5, "*RRA", rra, zero_page);
    set_instruction(0x77, 6, "*RRA", rra, zero_page_x);
    set_instruction(0x6F, 6, "*RRA", rra, absolute);
    set_instruction(0x7F, 7, "*RRA", rra, absolute_x);
    set_instruction(0x7B, 7, "*RRA", rra, absolute_y);
    set_instruction(0x63, 8, "*RRA", rra, indirect_x);
    set_instruction(0x73, 8, "*RRA", rra, indirect_y);

    /* RTI - Return from Interrupt. */
    set_instruction(0x40, 6, " RTI", rti, implied);

    /* RTS - Return from Subroutine. */
    set_instruction(0x60, 6, " RTS", rts, implied);

    /* SAX - Store Accumulator AND X. */
    set_instruction(0x87, 3, "*SAX", sax, zero_page);
    set_instruction(0x97, 4, "*SAX", sax, zero_page_y);
    set_instruction(0x8F, 4, "*SAX", sax, absolute);
    set_instruction(0x83, 6, "*SAX", sax, indirect_x); 

    /* SBC - Subtract with Carry. */
    set_instruction(0xE9, 2, " SBC", sbc, immediate);
    set_instruction(0xEB, 2, "*SBC", sbc, immediate);
    set_instruction(0xE5, 3, " SBC", sbc, zero_page);
    set_instruction(0xF5, 4, " SBC", sbc, zero_page_x);
    set_instruction(0xED, 4, " SBC", sbc, absolute);
    set_instruction(0xFD, 4, " SBC", sbc, absolute_x);
    set_instruction(0xF9, 4, " SBC", sbc, absolute_y);
    set_instruction(0xE1, 6, " SBC", sbc, indirect_x);
    set_instruction(0xF1, 5, " SBC", sbc, indirect_y);

    /* SEC - Set Carry Flag. */
    set_instruction(0x38, 2, " SEC", sec, implied);

    /* SEC - Set Decimal Flag. */
    set_instruction(0xF8, 2, " SED", sed, implied);

    /* SEI - Set Interrupt Disable. */
    set_instruction(0x78, 2, " SEI", sei, implied);

    /* SLO - Shift Left and Inclusive OR. */
    set_instruction(0x07, 5, "*SLO", slo, zero_page);
    set_instruction(0x17, 6, "*SLO", slo, zero_page_x);
    set_instruction(0x0F, 6, "*SLO", slo, absolute);
    set_instruction(0x1F, 7, "*SLO", slo, absolute_x);
    set_instruction(0x1B, 7, "*SLO", slo, absolute_y);
    set_instruction(0x03, 8, "*SLO", slo, indirect_x);
    set_instruction(0x13, 8, "*SLO", slo, indirect_y);

    /* SRE - Shift Right and Exclusive OR. */
    set_instruction(0x47, 5, "*SRE", sre, zero_page);
    set_instruction(0x57, 6, "*SRE", sre, zero_page_x);
    set_instruction(0x4F, 6, "*SRE", sre, absolute);
    set_instruction(0x5F, 7, "*SRE", sre, absolute_x);
    set_instruction(0x5B, 7, "*SRE", sre, absolute_y);
    set_instruction(0x43, 8, "*SRE", sre, indirect_x);
    set_instruction(0x53, 8, "*SRE", sre, indirect_y);

    /* STA - Store Accumulator. */
    set_instruction(0x85, 3, " STA", sta, zero_page);
    set_instruction(0x95, 4, " STA", sta, zero_page_x);
    set_instruction(0x8D, 4, " STA", sta, absolute);
    set_instruction(0x9D, 5, " STA", sta, absolute_x);
    set_instruction(0x99, 5, " STA", sta, absolute_y);
    set_instruction(0x81, 6, " STA", sta, indirect_x);
    set_instruction(0x91, 6, " STA", sta, indirect_y);

    /* STX - Store X Register. */
    set_instruction(0x86, 3, " STX", stx, zero_page);
    set_instruction(0x96, 4, " STX", stx, zero_page_y);
    set_instruction(0x8E, 4, " STX", stx, absolute);

    /* STY - Store Y Register. */
    set_instruction(0x84, 3, " STY", sty, zero_page);
    set_instruction(0x94, 4, " STY", sty, zero_page_x);
    set_instruction(0x8C, 4, " STY", sty, absolute);

    /* TAX - Transfer Accumulator to X. */
    set_instruction(0xAA, 2, " TAX", tax, implied);

    /* TAY - Transfer Accumulator to Y. */
    set_instruction(0xA8, 2, " TAY", tay, implied);

    /* TSX - Transfer Stack Pointer to X. */
    set_instruction(0xBA, 2, " TSX", tsx, implied);

    /* TSA - Transfer X to Accumulator. */
    set_instruction(0x8A, 2, " TXA", txa, implied);

    /* TXS - Transfer X to Stack Pointer. */
    set_instruction(0x9A, 2, " TXS", txs, implied);

    /* TYA - Transfer Y to Accumulator. */
    set_instruction(0x98, 2, " TYA", tya, implied);

    /* XAA - Transfer X to Accumulator and AND. */
    set_instruction(0x8B, 2, "*XAA", xaa, implied);
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
    wait_cycles = 0;
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
    wait_cycles = 0;

    /* Clear RAM. */
    for (int i = 0; i < RAM_SIZE; i++) {
        cpu.ram[i] = 0x00;
    }

    /* Clear all flags; IRQ disabled. */
    flg_reset();
    flg_set_I();
}

void cpu_cycle(int num_cycles) {
    while (num_cycles > 0) {
        if (wait_cycles == 0) {
            if (nmi) {
                interrupt(NMI_VECTOR);
                nmi = false;
            }
            else {
                extra_cycles = 0;
                opcode = mem_read(cpu.PC);
                LOG_CPU(9, "%04X  %02X", cpu.PC, opcode);
                (*cpu_addressing_table[opcode])();
                LOG_CPU(33, "A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%3d SL:%d\n",
                    cpu.A, cpu.X, cpu.Y, flg_get_status(false), cpu.S, ppu.dot,
                    ppu.scanline);
                (*cpu_instruction_table[opcode])();
                wait_cycles = cpu_cycles_table[opcode] + extra_cycles;
            }
        }

        wait_cycles--;
        num_cycles--;
        cycles++;
    }
}

inline unsigned long long cpu_get_ticks(void) {
    return cycles;
}

inline int cpu_get_wait_ticks(void) {
    return wait_cycles;
}

inline void cpu_suspend(int num_cycles) {
    wait_cycles += num_cycles;
}

inline byte cpu_ram_read(word address) {
    return cpu.ram[address];
}

inline void cpu_ram_write(word address, byte data) {
    cpu.ram[address] = data;
}
