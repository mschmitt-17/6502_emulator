#ifndef _6502_H
#define _6502_H

#include <stdint.h>

#include "assembler/bytecode.h"

#define MEMORY_SIZE     (65536)
#define IRQ_ADDRESS     (0xFFFE)

/*
 * opcodes are 8 bits long and have the general form AAABBBCC
 * AAA and CC define the opcode
 * BBB defines the addressing mode
 */
#define AAA_BITMASK     (0xE0)
#define BBB_BITMASK     (0x1C)
#define CC_BITMASK      (0x03)

/*  STATUS (flags) REGISTER BITMAPPING
 *  7 6 5 4 3 2 1 0
 *  N V   B D I Z C
 *      C (Carry Flag): holds carry out of MSB in any arithmetic operation
 *      Z (Zero Flag): set to 1 whenever arithmetic/logical operation has 0 as result
 *      I (Interrupt Flag): cleared = interrupts enabled
 *      D (Decimal Flag): when set, arithmetic is binary-coded decimal (BCD)
 *      B (BRK Flag): set when software interrupt is executed
 *      V (Overflow Flag): when an arithmetic operation produces a result too large for a byte
 *      N (Sign Flag): set if the result of an operation is negative
 */
#define CARRY_INDEX         (0)
#define ZERO_INDEX          (1)
#define INTERRUPT_INDEX     (2)
#define DECIMAL_INDEX       (3)
#define BREAK_INDEX         (4)
#define OVERFLOW_INDEX      (6)
#define NEGATIVE_INDEX      (7)

/* SPECIAL CASE OPCODES (one addressing mode) */
#define OP_BRK      (0x00) // forced interrupt
#define OP_PHP      (0x08) // push processor status on stack
#define OP_BPL      (0x10) // branch on N = 0
#define OP_CLC      (0x18) // clear carry flag
#define OP_JSR      (0x20) // jump to subroutine after pushing return address to stack
#define OP_JSR_GEN  (0x24) // used since JSR shares opcode with BIT
#define OP_PLP      (0x28) // pull processor status from stack
#define OP_BMI      (0x30) // branch on N = 1
#define OP_SEC      (0x38) // set carry flag
#define OP_RTI      (0x40) // return from interrupt
#define OP_PHA      (0x48) // push accumulator to stack
#define OP_JMP      (0x4C) // jump to new location
#define OP_BVC      (0x50) // branch on V = 0
#define OP_CLI      (0x58) // clear interrupt disable flag
#define OP_RTS      (0x60) // return from subroutine
#define OP_PLA      (0x68) // pull accumulator from stack
#define OP_JI       (0x6C) // jump to new location (indirect addressing)
#define OP_BVS      (0x70) // branch on V = 1
#define OP_SEI      (0x78) // set interrupt disable flag
#define OP_DEY      (0x88) // decrement index y by one
#define OP_TXA      (0x8A) // transfer index x to accumulator
#define OP_BCC      (0x90) // branch on carry clear
#define OP_TYA      (0x98) // transfer index y to accumulator
#define OP_TXS      (0x9A) // transfer index x to stack pointer
#define OP_TAY      (0xA8) // transfer accumulator to index y
#define OP_TAX      (0xAA) // transfer accumulator to index x
#define OP_BCS      (0xB0) // branch on carry set
#define OP_CLV      (0xB8) // clear overflow flag
#define OP_TSX      (0xBA) // transfer stack pointer to index x
#define OP_INY      (0xC8) // increment index y by 1
#define OP_DEX      (0xCA) // decrement index x by 1
#define OP_BNE      (0xD0) // branch on zero flag clear
#define OP_CLD      (0xD8) // clear decimal flag
#define OP_INX      (0xE8) // increment index x by 1
#define OP_NOP      (0xEA) // no op
#define OP_BEQ      (0xF0) // branch on zero flag set
#define OP_SED      (0xF8) // set decimal mode

/* NORMAL OPCODES (multiple addressing modes) */
#define OP_ORA      (0x01) // or memory (determined by addressing mode) with accumulator, result stored in accumulator
#define OP_ASL      (0x02) // arithmetic shift left one bit
#define OP_BIT      (0x20) // test bits in memory with accumulator
#define OP_AND      (0x21) // and memory with accumulator
#define OP_ROL      (0x22) // rotate one bit left
#define OP_EOR      (0x41) // xor memory with accumulator
#define OP_LSR      (0x42) // shift right one bit
#define OP_ADC      (0x61) // add memory to accumulator with carry
#define OP_ROR      (0x62) // rotate one bit right
#define OP_STY      (0x80) // store index Y in memory
#define OP_STA      (0x81) // store accumulator in memory
#define OP_STX      (0x82) // store index X in memory
#define OP_LDY      (0xA0) // load index Y with memory
#define OP_LDA      (0xA1) // load accumulator with memory
#define OP_LDX      (0xA2) // load index X with memory
#define OP_CPY      (0xC0) // compare memory and index Y
#define OP_CMP      (0xC1) // compare memory and accumulator
#define OP_DEC      (0xC2) // decrement memory by one
#define OP_CPX      (0xE0) // compare memory and index X
#define OP_SBC      (0xE1) // subtract memory from accumulator with borrow
#define OP_INC      (0xE2) // increment memory by one

/* ADDRESSING MODES (obtained using BBB_BITMASK and right shifting) */
#define ADDR_MODE_IND_X             (0x00)
#define ADDR_MODE_ZPG               (0x01)
#define ADDR_MODE_IMM               (0x02)
#define ADDR_MODE_ACCUM             (0x02)
#define ADDR_MODE_ABS               (0x03)
#define ADDR_MODE_IND_Y             (0x04)
#define ADDR_MODE_ZPG_X             (0x05)
#define ADDR_MODE_ZPG_Y             (0x05) // opcodes that store or load x_index use this mode
#define ADDR_MODE_ABS_Y             (0x06)
#define ADDR_MODE_ABS_X             (0x07)
#define ADDR_MODE_IMP               (0x08) // for parsing only
#define ADDR_MODE_ACCUM_GEN        (0x09) // for parsing only
#define ADDR_MODE_ZPG_Y_GEN        (0x0A) // for parsing only
#define ADDR_MODE_REL               (0x0B) // addressing for OPC LABEL; not necessarily relative
#define ADDR_MODE_IND               (0x0E) // only for indirect jump instruction


/* MEMORY ACCESS MACROS */
#define IND_X_MEM_ACCESS    (sf->memory[((sf->memory[((sf->x_index + sf->memory[sf->pc + 1]) + 1) % 0xFF] << 8) | (sf->memory[(sf->x_index + sf->memory[sf->pc + 1]) % 0xFF]))]) // add x_index without carry
#define ZPG_MEM_ACCESS      (sf->memory[sf->memory[sf->pc + 1]])
#define IMM_MEM_ACCESS      (sf->memory[sf->pc + 1])
#define ABS_MEM_ACCESS      (sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]])
#define IND_Y_MEM_ACCESS    (sf->memory[((sf->memory[sf->memory[sf->pc + 1] + 1] << 8)|sf->memory[sf->memory[sf->pc + 1]]) + sf->y_index])
#define ZPG_X_MEM_ACCESS    (sf->memory[(sf->memory[sf->pc + 1] + sf->x_index) % 0xFF]) // add x_index without carry
#define ZPG_Y_MEM_ACCESS    (sf->memory[(sf->memory[sf->pc + 1] + sf->y_index) % 0xFF]) // add x_index without carry
#define ABS_Y_MEM_ACCESS    (sf->memory[(((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->y_index) % 0xFFFF]) // has carry, but can't have result outside address space
#define ABS_X_MEM_ACCESS    (sf->memory[(((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->x_index) % 0xFFFF])


typedef struct sf {
    uint8_t accumulator;
    uint8_t x_index;
    uint8_t y_index;
    uint8_t status;
    uint16_t esp;
    uint16_t pc;
    uint8_t memory[MEMORY_SIZE];
} sf_t;

/* loads passed number of bytes from bytecode to passed load address */
void load_bytecode(sf_t *sf, Bytecode_t *bc, uint16_t load_address, uint32_t num_bytes);

/* initializes registers to all zeroes, to be called before game is run */
void initialize_regs(sf_t *sf, uint16_t pc_init);

/* executes one instruction (pass sf since different addressing = different bytes for instruction) */
void process_line(sf_t *sf);

#endif
