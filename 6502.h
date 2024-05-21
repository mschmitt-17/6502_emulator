#ifndef _6502_H
#define _6502_H

#include "stdint.h"

#define MEMORY_SIZE     65536
#define IRQ_ADDRESS     0xFFFE
#define AAA_BITMASK     0xE0
#define CC_BITMASK      0x03
#define CARRY_INDEX         0
#define ZERO_INDEX          1
#define INTERRUPT_INDEX     2
#define DECIMAL_INDEX       3
#define BREAK_INDEX         4
#define OVERFLOW_INDEX      6
#define NEGATIVE_INDEX      7

/*
 * 6502 memory map according to ChatGPT:
 *      0x0000 - 0x00FF: Zero Page
 *      0x0100 - 0x01FF: Stack
 *      0x0200 - 0x07FF: System Area
 *      0x8000 - 0xBFFF: Cartridge ROM
 *      0xC000 - 0xFFFF: Cartridge RAM
 */
#define ZERO_PAGE_START     0x0000
#define STACK_START         0x01FF
#define SYSTEM_START        0x0200
#define ROM_START           0x8000
#define RAM_START           0xC000

/* OPCODES */
#define OP_BRK      0x00 // forced interrupt
#define OP_PHP      0x08 // push processor status on stack
#define OP_BPL      0x10 // branch on N = 0
#define OP_CLC      0x18 // clear carry flag
#define OP_JSR      0x20 // jump to subroutine after pushing return address to stack
#define OP_PLP      0x28 // pull processor status from stack
#define OP_BMI      0x30 // branch on N = 1
#define OP_SEC      0x38 // 
#define OP_RTI      0x40 // return from interrupt
#define OP_PHA      0x48 // 
#define OP_JMP      0x4C // 
#define OP_BVC      0x50 //
#define OP_CLI      0x58 // 
#define OP_RTS      0x60 // return from subroutine
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_
#define OP_


typedef struct sf {
    uint8_t accumulator;
    uint8_t x_index;
    uint8_t y_index;
    /*  7 6 5 4 3 2 1 0
     *  N V   B D I Z C
     *      C (Carry Flag): holds carry out of MSB in any arithmetic operation
     *      Z (Zero Flag): set to 1 whenever arithmetic/logical operation has 0 as result
     *      I (Interrupt Flag): cleared = interrupts enabled
     *      D (Decimal Flag): when set, arithmetic is binary-coded decimal (BCD)
     *      B (BRK Flag): set when software interrupt is executed
     *      V (Overflow Flag): when an arithmetic operation produces a result too large for a byte
     *      N (Sign Flag): set if the result of an operation is negative
     */
    uint8_t status;
    uint16_t esp;
    uint16_t pc;
    uint8_t memory[MEMORY_SIZE];
} sf_t;

/* initializes registers to all zeroes, to be called before game is run */
void initialize_regs(sf_t *sf);

/* executes one instruction (pass sf since different addressing = different bytes for instruction) */
void process_line(sf_t *sf);

#endif
