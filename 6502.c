#include "6502.h"
#include "string.h"

// maybe change this to load_rom function since we want to reset everything when starting new game
void initialize_regs(sf_t *sf) {
    sf->esp = STACK_START; // stack grows down
    sf->pc = ROM_START; // pc should begin where ROM begins
    sf->accumulator = 0;
    sf->x_index = 0;
    sf->y_index = 0;
    sf->status = 0;
    memset(sf->memory, 0, MEMORY_SIZE);
}

/*
 * opcodes are 8 bits long and have the general form AAABBBCC
 * AAA and CC define the opcode
 * BBB defines the addressing mode
 */

// since number of bytes needed will vary based on operation and addressing mode,
// process data "line-by-line" instead of opcode-by-opcode
void process_line (sf_t *sf) {
    switch (sf->memory[sf->pc]) { 
        case OP_BRK:
            sf->memory[sf->esp--] = (sf->pc+1) & 0x00FF; // push low byte of PC for next instruction
            sf->memory[sf->esp--] = (sf->pc+1) >> 8; // push high byte of PC for next instruction
            sf->memory[sf->esp--] = sf->status; // push flags
            sf->status |= (1 << INTERRUPT_INDEX);
            sf->status |= (1 << BREAK_INDEX);
            sf->pc = IRQ_ADDRESS;
            break;
        
        case OP_PHP:
            sf->memory[sf->esp--] = sf->status;
            sf->pc++;
            break;
        
        case OP_BPL:
            if (!(sf->status & (1 << NEGATIVE_INDEX))) {
                sf->pc += sf->memory[sf->pc + 1]; // offset is stored in following byte
            } else {
                sf->pc += 2; // skip offset
            }
            break;
        
        case OP_CLC:
            sf->status &= ~(1 << CARRY_INDEX);
            sf->pc++;
            break;
        
        case OP_JSR:
            // push return address to stack
            sf->memory[sf->esp--] = (sf->pc + 3) & 0x00FF;
            sf->memory[sf->esp--] = (sf->pc + 3) >> 8;
            // set pc to address provided to jump instruction
            sf->pc = (sf->memory[sf->pc+2] << 8)|sf->memory[sf->pc+1];
            break;

        case OP_PLP:
            sf->status = sf->memory[++sf->esp];
            sf->pc++;
            break;

        case OP_BMI:
            if (sf->status & (1 << NEGATIVE_INDEX)) {
                sf->pc += sf->memory[sf->pc + 1];
            } else {
                sf->pc += 2;
            }
            break;

        case OP_SEC:
            sf->status |= (1 << CARRY_INDEX);
            sf->pc++;
            break;

        case OP_RTI:
            sf->status = sf->memory[++sf->esp];
            sf->pc = sf->memory[++sf->esp] << 8;
            sf->pc |= sf->memory[++sf->esp];
            // don't increment PC after this because in BRK we push PC for next instruction
            break;

        case OP_PHA:
            sf->memory[sf->esp--] = sf->accumulator;
            sf->pc++;
            break;

        case OP_JMP:
            sf->pc = (sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1];
            break;

        case OP_BVC:
            if (!(sf->status & (1 << OVERFLOW_INDEX))) {
                sf->pc += sf->memory[sf->pc + 1];
            } else {
                sf->pc += 2;
            }
            break;

        case OP_CLI:
            sf->status &= ~(1 << INTERRUPT_INDEX);
            sf->pc++;
            break;

        case OP_RTS:
            sf->pc = sf->memory[++sf->esp] << 8;
            sf->pc |= sf->memory[++sf->esp];
            // don't increment PC after this because in JSR we push PC for next instruction
            break;

        case OP_PLA:
            sf->accumulator = sf->memory[++sf->esp];
            sf->pc++;
            break;
        
        case OP_JI:
            sf->pc = ((sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + 1]) << 8) |
                        (sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]]);
            break;

        case OP_BVS:
            if (sf->status & (1 << OVERFLOW_INDEX)) {
                sf->pc += sf->memory[sf->pc + 1];
            } else {
                sf->pc += 2;
            }
            break;
        // SEI
        case OP_SEI:
            sf->status |= (1 << INTERRUPT_INDEX);
            sf->pc++;
            break;
        // DEY
        case 0x88:
            break;
        // TXA
        case 0x8A:
            break;
        // BCC
        case 0x90:
            break;
        // TYA
        case 0x98:
            break;
        // TXS
        case 0x9A:
            break;
        // TAY
        case 0xA8:
            break;
        // TAX
        case 0xAA:
            break;
        // BCS
        case 0xB0:
            break;

        case OP_CLV:
            sf->status &= ~(1 << OVERFLOW_INDEX);
            sf->pc++;
            break;
        // TSX
        case 0xBA:
            break;
        // INY
        case 0xC8:
            break;
        // DEX
        case 0xCA:
            break;
        // BNE
        case 0xD0:
            break;

        case OP_CLD:
            sf->status &= ~(1 << DECIMAL_INDEX);
            sf->pc++;
            break;
        // INX
        case 0xE8:
            break;
        // NOP
        case 0xEA:
            break;
        // BEQ
        case 0xF0:
            break;
        // SED
        case OP_SED:
            sf->status |= (1 << DECIMAL_INDEX);
            sf->pc++;
            break;
        default:
            // switch case for other shit here
            switch (((sf->memory[sf->pc] & AAA_BITMASK) >> 3) | sf->memory[sf->pc] & CC_BITMASK) {
                // OR
                case 0x01:
                    break;
                // ASL
                case 0x02:
                // BIT
                case 0x04:
                // AND
                case 0x05:
                // ROL
                case 0x06:
                // EOR
                case 0x09:
                // LSR
                case 0x0A:
                // ADC
                case 0x0D:
                // ROR
                case 0x0E:
                // STY
                case 0x10:
                // STA
                case 0x11:
                // STX
                case 0x12:
                // LDY
                case 0x14:
                // LDA
                case 0x15:
                // LDX
                case 0x16:
                // CPY
                case 0x18:
                // CMP
                case 0x19:
                // DEC
                case 0x1A:
                // CPX
                case 0x1C:
                // SBC
                case 0x1D:
                // INC
                case 0x1E:
            }
            break;

    }
}
