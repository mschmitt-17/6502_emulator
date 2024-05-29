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

        case OP_SEI:
            sf->status |= (1 << INTERRUPT_INDEX);
            sf->pc++;
            break;

        case OP_DEY:
            sf->y_index--;
            sf->pc++;
            break;

        case OP_TXA:
            sf->accumulator = sf->x_index;
            sf->pc++;
            break;

        case OP_BCC:
            if (!(sf->status & (1 << CARRY_INDEX))) {
                sf->pc += sf->memory[sf->pc + 1];
            } else {
                sf->pc += 2;
            }
            break;

        case OP_TYA:
            sf->accumulator = sf->y_index;
            sf->pc++;
            break;

        case OP_TXS:
            sf->esp = sf->x_index;
            sf->pc++;
            break;

        case OP_TAY:
            sf->y_index = sf->accumulator;
            sf->pc++;
            break;

        case OP_TAX:
            sf->x_index = sf->accumulator;
            sf->pc++;
            break;

        case OP_BCS:
            if (sf->status & (1 << CARRY_INDEX)) {
                sf->pc += sf->memory[sf->pc + 1];
            } else {
                sf->pc += 2;
            }
            break;

        case OP_CLV:
            sf->status &= ~(1 << OVERFLOW_INDEX);
            sf->pc++;
            break;

        case OP_TSX:
            sf->x_index = sf->esp;
            sf->pc++;
            break;

        case OP_INY:
            sf->y_index++;
            sf->pc++;
            break;

        case OP_DEX:
            sf->x_index--;
            sf->pc++;
            break;

        case OP_BNE:
            if (!(sf->status & (1 << ZERO_INDEX))) {
                sf->pc += sf->memory[sf->pc + 1];
            } else {
                sf->pc += 2;
            }
            break;

        case OP_CLD:
            sf->status &= ~(1 << DECIMAL_INDEX);
            sf->pc++;
            break;

        case OP_INX:
            sf->x_index++;
            sf->pc++;
            break;

        case OP_NOP:
            sf->pc++;
            break;

        case OP_BEQ:
            if (sf->status & (1 << ZERO_INDEX)) {
                sf->pc += sf->memory[sf->pc + 1];
            } else {
                sf->pc += 2;
            }
            break;

        case OP_SED:
            sf->status |= (1 << DECIMAL_INDEX);
            sf->pc++;
            break;

        default:
            // switch case for other shit here
            switch (((sf->memory[sf->pc] & AAA_BITMASK) >> 3) | sf->memory[sf->pc] & CC_BITMASK) {
                case OP_ORA:
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            sf->accumulator = sf->accumulator |
                                              sf->memory[((sf->memory[((sf->x_index + sf->memory[sf->pc + 1]) + 1) % 0xFF] << 8) |
                                               (sf->memory[(sf->x_index + sf->memory[sf->pc + 1]) % 0xFF]))];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG:
                            sf->accumulator = sf->accumulator | sf->memory[sf->memory[sf->pc + 1]];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_IMM:
                            sf->accumulator = sf->accumulator | sf->memory[sf->pc + 1];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            sf->accumulator = sf->accumulator | (sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]]);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_IND_Y:
                            sf->accumulator = sf->accumulator |
                                              sf->memory[((sf->memory[sf->memory[sf->pc + 1] + 1] << 8)|
                                               sf->memory[sf->memory[sf->pc + 1]]) + sf->y_index];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG_X:
                            sf->accumulator = (sf->accumulator|sf->memory[sf->memory[sf->pc + 1] + sf->x_index]);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_Y:
                            sf->accumulator = sf->accumulator | sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->y_index];
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ABS_X:
                            sf->accumulator = sf->accumulator | sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->x_index];
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    break;

                case OP_ASL:
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            break;
                        case ADDR_MODE_ZPG:
                            break;
                        case ADDR_MODE_IMM:
                            break;
                        case ADDR_MODE_ABS:
                            break;
                        case ADDR_MODE_IND_Y:
                            break;
                        case ADDR_MODE_ZPG_X:
                            break;
                        case ADDR_MODE_ABS_Y:
                            break;
                        case ADDR_MODE_ABS_X:
                            break;
                        default:
                            break;
                    }
                    break;

                case OP_BIT:
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            break;
                        case ADDR_MODE_ZPG:
                            break;
                        case ADDR_MODE_IMM:
                            break;
                        case ADDR_MODE_ABS:
                            break;
                        case ADDR_MODE_IND_Y:
                            break;
                        case ADDR_MODE_ZPG_X:
                            break;
                        case ADDR_MODE_ABS_Y:
                            break;
                        case ADDR_MODE_ABS_X:
                            break;
                        default:
                            break;
                    }
                    break;

                case OP_AND:
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            break;
                        case ADDR_MODE_ZPG:
                            break;
                        case ADDR_MODE_IMM:
                            break;
                        case ADDR_MODE_ABS:
                            break;
                        case ADDR_MODE_IND_Y:
                            break;
                        case ADDR_MODE_ZPG_X:
                            break;
                        case ADDR_MODE_ABS_Y:
                            break;
                        case ADDR_MODE_ABS_X:
                            break;
                        default:
                            break;
                    }
                    break;

                case OP_ROL:
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            break;
                        case ADDR_MODE_ZPG:
                            break;
                        case ADDR_MODE_IMM:
                            break;
                        case ADDR_MODE_ABS:
                            break;
                        case ADDR_MODE_IND_Y:
                            break;
                        case ADDR_MODE_ZPG_X:
                            break;
                        case ADDR_MODE_ABS_Y:
                            break;
                        case ADDR_MODE_ABS_X:
                            break;
                        default:
                            break;
                    }
                    break;

                case OP_EOR:
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            break;
                        case ADDR_MODE_ZPG:
                            break;
                        case ADDR_MODE_IMM:
                            break;
                        case ADDR_MODE_ABS:
                            break;
                        case ADDR_MODE_IND_Y:
                            break;
                        case ADDR_MODE_ZPG_X:
                            break;
                        case ADDR_MODE_ABS_Y:
                            break;
                        case ADDR_MODE_ABS_X:
                            break;
                        default:
                            break;
                    }
                    break;

                case OP_LSR:
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            break;
                        case ADDR_MODE_ZPG:
                            break;
                        case ADDR_MODE_IMM:
                            break;
                        case ADDR_MODE_ABS:
                            break;
                        case ADDR_MODE_IND_Y:
                            break;
                        case ADDR_MODE_ZPG_X:
                            break;
                        case ADDR_MODE_ABS_Y:
                            break;
                        case ADDR_MODE_ABS_X:
                            break;
                        default:
                            break;
                    }
                    break;

                case OP_ADC:
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            break;
                        case ADDR_MODE_ZPG:
                            break;
                        case ADDR_MODE_IMM:
                            break;
                        case ADDR_MODE_ABS:
                            break;
                        case ADDR_MODE_IND_Y:
                            break;
                        case ADDR_MODE_ZPG_X:
                            break;
                        case ADDR_MODE_ABS_Y:
                            break;
                        case ADDR_MODE_ABS_X:
                            break;
                        default:
                            break;
                    }
                    break;

                case OP_ROR:
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            break;
                        case ADDR_MODE_ZPG:
                            break;
                        case ADDR_MODE_IMM:
                            break;
                        case ADDR_MODE_ABS:
                            break;
                        case ADDR_MODE_IND_Y:
                            break;
                        case ADDR_MODE_ZPG_X:
                            break;
                        case ADDR_MODE_ABS_Y:
                            break;
                        case ADDR_MODE_ABS_X:
                            break;
                        default:
                            break;
                    }
                    break;

                case OP_STY:
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            break;
                        case ADDR_MODE_ZPG:
                            break;
                        case ADDR_MODE_IMM:
                            break;
                        case ADDR_MODE_ABS:
                            break;
                        case ADDR_MODE_IND_Y:
                            break;
                        case ADDR_MODE_ZPG_X:
                            break;
                        case ADDR_MODE_ABS_Y:
                            break;
                        case ADDR_MODE_ABS_X:
                            break;
                        default:
                            break;
                    }
                    break;

                case OP_STA:
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            break;
                        case ADDR_MODE_ZPG:
                            break;
                        case ADDR_MODE_IMM:
                            break;
                        case ADDR_MODE_ABS:
                            break;
                        case ADDR_MODE_IND_Y:
                            break;
                        case ADDR_MODE_ZPG_X:
                            break;
                        case ADDR_MODE_ABS_Y:
                            break;
                        case ADDR_MODE_ABS_X:
                            break;
                        default:
                            break;
                    }
                    break;

                case OP_STX:
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            break;
                        case ADDR_MODE_ZPG:
                            break;
                        case ADDR_MODE_IMM:
                            break;
                        case ADDR_MODE_ABS:
                            break;
                        case ADDR_MODE_IND_Y:
                            break;
                        case ADDR_MODE_ZPG_X:
                            break;
                        case ADDR_MODE_ABS_Y:
                            break;
                        case ADDR_MODE_ABS_X:
                            break;
                        default:
                            break;
                    }
                    break;

                case OP_LDY:
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            break;
                        case ADDR_MODE_ZPG:
                            break;
                        case ADDR_MODE_IMM:
                            break;
                        case ADDR_MODE_ABS:
                            break;
                        case ADDR_MODE_IND_Y:
                            break;
                        case ADDR_MODE_ZPG_X:
                            break;
                        case ADDR_MODE_ABS_Y:
                            break;
                        case ADDR_MODE_ABS_X:
                            break;
                        default:
                            break;
                    }
                    break;

                case OP_LDA:
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            break;
                        case ADDR_MODE_ZPG:
                            break;
                        case ADDR_MODE_IMM:
                            break;
                        case ADDR_MODE_ABS:
                            break;
                        case ADDR_MODE_IND_Y:
                            break;
                        case ADDR_MODE_ZPG_X:
                            break;
                        case ADDR_MODE_ABS_Y:
                            break;
                        case ADDR_MODE_ABS_X:
                            break;
                        default:
                            break;
                    }
                    break;

                case OP_LDX:
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            break;
                        case ADDR_MODE_ZPG:
                            break;
                        case ADDR_MODE_IMM:
                            break;
                        case ADDR_MODE_ABS:
                            break;
                        case ADDR_MODE_IND_Y:
                            break;
                        case ADDR_MODE_ZPG_X:
                            break;
                        case ADDR_MODE_ABS_Y:
                            break;
                        case ADDR_MODE_ABS_X:
                            break;
                        default:
                            break;
                    }
                    break;

                case OP_CPY:
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            break;
                        case ADDR_MODE_ZPG:
                            break;
                        case ADDR_MODE_IMM:
                            break;
                        case ADDR_MODE_ABS:
                            break;
                        case ADDR_MODE_IND_Y:
                            break;
                        case ADDR_MODE_ZPG_X:
                            break;
                        case ADDR_MODE_ABS_Y:
                            break;
                        case ADDR_MODE_ABS_X:
                            break;
                        default:
                            break;
                    }
                    break;

                case OP_CMP:
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            break;
                        case ADDR_MODE_ZPG:
                            break;
                        case ADDR_MODE_IMM:
                            break;
                        case ADDR_MODE_ABS:
                            break;
                        case ADDR_MODE_IND_Y:
                            break;
                        case ADDR_MODE_ZPG_X:
                            break;
                        case ADDR_MODE_ABS_Y:
                            break;
                        case ADDR_MODE_ABS_X:
                            break;
                        default:
                            break;
                    }
                    break;

                case OP_DEC:
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            break;
                        case ADDR_MODE_ZPG:
                            break;
                        case ADDR_MODE_IMM:
                            break;
                        case ADDR_MODE_ABS:
                            break;
                        case ADDR_MODE_IND_Y:
                            break;
                        case ADDR_MODE_ZPG_X:
                            break;
                        case ADDR_MODE_ABS_Y:
                            break;
                        case ADDR_MODE_ABS_X:
                            break;
                        default:
                            break;
                    }
                    break;

                case OP_CPX:
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            break;
                        case ADDR_MODE_ZPG:
                            break;
                        case ADDR_MODE_IMM:
                            break;
                        case ADDR_MODE_ABS:
                            break;
                        case ADDR_MODE_IND_Y:
                            break;
                        case ADDR_MODE_ZPG_X:
                            break;
                        case ADDR_MODE_ABS_Y:
                            break;
                        case ADDR_MODE_ABS_X:
                            break;
                        default:
                            break;
                    }
                    break;

                case OP_SBC:
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            break;
                        case ADDR_MODE_ZPG:
                            break;
                        case ADDR_MODE_IMM:
                            break;
                        case ADDR_MODE_ABS:
                            break;
                        case ADDR_MODE_IND_Y:
                            break;
                        case ADDR_MODE_ZPG_X:
                            break;
                        case ADDR_MODE_ABS_Y:
                            break;
                        case ADDR_MODE_ABS_X:
                            break;
                        default:
                            break;
                    }
                    break;

                case OP_INC:
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            break;
                        case ADDR_MODE_ZPG:
                            break;
                        case ADDR_MODE_IMM:
                            break;
                        case ADDR_MODE_ABS:
                            break;
                        case ADDR_MODE_IND_Y:
                            break;
                        case ADDR_MODE_ZPG_X:
                            break;
                        case ADDR_MODE_ABS_Y:
                            break;
                        case ADDR_MODE_ABS_X:
                            break;
                        default:
                            break;
                    }
                    break;
            }
            break;

    }
}
