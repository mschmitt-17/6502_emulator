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
            switch (((sf->memory[sf->pc] & AAA_BITMASK) >> 3) | (sf->memory[sf->pc] & CC_BITMASK)) {
                case ((OP_ORA & AAA_BITMASK) >> 3) | (OP_ORA & CC_BITMASK):
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
                            sf->accumulator = (sf->accumulator | sf->memory[sf->memory[sf->pc + 1] + sf->x_index]);
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
                        // adjust flags
                        if (sf->accumulator < 0) {
                            sf->status |= (1 << NEGATIVE_INDEX);
                        } else if (sf->accumulator == 0) {
                            sf->status |= (1 << ZERO_INDEX);
                        }
                    }
                    break;
                // TODO: fix negatives
                case ((OP_ASL & AAA_BITMASK) >> 3) | (OP_ASL & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_ZPG:
                            // set carry flag before performing operation to avoid converting type
                            if (sf->memory[sf->memory[sf->pc + 1]] & 0x80) {
                                sf->status |= (1 << CARRY_INDEX);
                            }
                            sf->memory[sf->memory[sf->pc + 1]] = sf->memory[sf->memory[sf->pc + 1]] << 1;
                            if (sf->memory[sf->memory[sf->pc + 1]] & 0x80) {
                                sf->status |= (1 << NEGATIVE_INDEX);
                            } else if (sf->memory[sf->memory[sf->pc + 1]] == 0) {
                                sf->status != (1 << ZERO_INDEX);
                            }
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ACCUM:
                            if (sf->accumulator & 0x80) {
                                sf->status |= (1 << CARRY_INDEX);
                            }
                            sf->accumulator = sf->accumulator << 1;
                            if (sf->accumulator & 0x80) {
                                sf->status |= (1 << NEGATIVE_INDEX);
                            } else if (sf->accumulator == 0) {
                                sf->status != (1 << ZERO_INDEX);
                            }
                            sf->pc += 1;
                            break;
                        case ADDR_MODE_ABS:
                            if (sf->memory[(sf->memory[sf->pc + 2] << 8) | sf->memory[sf->pc + 1]] & 0x80) {
                                sf->status |= (1 << CARRY_INDEX);
                            }
                            sf->memory[(sf->memory[sf->pc + 2] << 8) | sf->memory[sf->pc + 1]] = sf->memory[(sf->memory[sf->pc + 2] << 8) | sf->memory[sf->pc + 1]] << 1;
                            if (sf->memory[(sf->memory[sf->pc + 2] << 8) | sf->memory[sf->pc + 1]] & 0x80) {
                                sf->status |= (1 << NEGATIVE_INDEX);
                            } else if (sf->memory[(sf->memory[sf->pc + 2] << 8) | sf->memory[sf->pc + 1]] == 0) {
                                sf->status |= (1 << ZERO_INDEX);
                            }
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ZPG_X:
                            if (sf->memory[(uint16_t)sf->memory[sf->pc + 1] + sf->x_index] & 0x80) {
                                sf->status |= (1 << CARRY_INDEX);
                            }
                            sf->memory[(uint16_t)sf->memory[sf->pc + 1] + sf->x_index] = sf->memory[(uint16_t)sf->memory[sf->pc + 1] + sf->x_index] << 1;
                            if (sf->memory[(uint16_t)sf->memory[sf->pc + 1] + sf->x_index] & 0x80) {
                                sf->status |= (1 << NEGATIVE_INDEX);
                            } else if (sf->memory[(uint16_t)sf->memory[sf->pc + 1] + sf->x_index] == 0) {
                                sf->status != (1 << ZERO_INDEX);
                            }
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_X:
                            if (sf->memory[((sf->memory[sf->pc + 2] << 8) | sf->memory[sf->pc + 1]) + sf->x_index] & 0x80) {
                                sf->status |= (1 << CARRY_INDEX);
                            }
                            sf->memory[((sf->memory[sf->pc + 2] << 8) | sf->memory[sf->pc + 1]) + sf->x_index] = sf->memory[((sf->memory[sf->pc + 2] << 8) | sf->memory[sf->pc + 1]) + sf->x_index] << 1;
                            if (sf->memory[((sf->memory[sf->pc + 2] << 8) | sf->memory[sf->pc + 1]) + sf->x_index] & 0x80) {
                                sf->status |= (1 << NEGATIVE_INDEX);
                            } else if (sf->memory[((sf->memory[sf->pc + 2] << 8) | sf->memory[sf->pc + 1]) + sf->x_index] == 0) {
                                sf->status |= (1 << ZERO_INDEX);
                            }
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    break;

                case ((OP_BIT & AAA_BITMASK) >> 3) | (OP_BIT & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_ZPG:
                            if ((sf->accumulator & sf->memory[sf->memory[sf->pc + 1]]) == 0) {
                                sf->status |= (1 << ZERO_INDEX);
                            }
                            sf->status |= sf->memory[sf->memory[sf->pc + 1]] & (1 << NEGATIVE_INDEX);
                            sf->status |= sf->memory[sf->memory[sf->pc + 1]] & (1 << OVERFLOW_INDEX);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            if (sf->accumulator & (sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]]) == 0) {
                                sf->status |= (1 << ZERO_INDEX);
                            }
                            sf->status |= sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]] & (1 << NEGATIVE_INDEX);
                            sf->status |= sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]] & (1 << OVERFLOW_INDEX);
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    break;

                case ((OP_AND & AAA_BITMASK) >> 3) | (OP_AND & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            sf->accumulator = sf->accumulator &
                                              sf->memory[((sf->memory[((sf->x_index + sf->memory[sf->pc + 1]) + 1) % 0xFF] << 8) |
                                               (sf->memory[(sf->x_index + sf->memory[sf->pc + 1]) % 0xFF]))];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG:
                            sf->accumulator = sf->accumulator & sf->memory[sf->memory[sf->pc + 1]];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_IMM:
                            sf->accumulator = sf->accumulator & sf->memory[sf->pc + 1];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            sf->accumulator = sf->accumulator & (sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]]);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_IND_Y:
                            sf->accumulator = sf->accumulator &
                                              sf->memory[((sf->memory[sf->memory[sf->pc + 1] + 1] << 8)|
                                               sf->memory[sf->memory[sf->pc + 1]]) + sf->y_index];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG_X:
                            sf->accumulator = (sf->accumulator & sf->memory[sf->memory[sf->pc + 1] + sf->x_index]);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_Y:
                            sf->accumulator = sf->accumulator & sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->y_index];
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ABS_X:
                            sf->accumulator = sf->accumulator & sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->x_index];
                            sf->pc += 3;
                            break;
                        default:
                            break;
                        // adjust flags
                    }
                    if (sf->accumulator & 0x80) {
                        sf->status |= (1 << NEGATIVE_INDEX);
                    } else if (sf->accumulator == 0) {
                        sf->status |= (1 << ZERO_INDEX);
                    }
                    break;

                case ((OP_ROL & AAA_BITMASK) >> 3) | (OP_ROL & CC_BITMASK):
                    uint16_t temp_ROL;
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_ZPG:
                            temp_ROL = (uint16_t)sf->memory[sf->memory[sf->pc + 1]]; 
                            temp_ROL = temp_ROL << 1;
                            // check for shift in a 1
                            if (sf->status & (1 << CARRY_INDEX)) {
                                temp_ROL += 1;
                            }
                            // check for shift into carry
                            if (temp_ROL & 0x100) {
                                sf->status |= (1 << CARRY_INDEX);
                            }
                            // check for negative/zero
                            if (temp_ROL & 0x80) {
                                sf->status |= (1 << NEGATIVE_INDEX);
                            } else if ((temp_ROL & 0xFF) == 0) {
                                sf->status |= (1 << ZERO_INDEX);
                            }
                            sf->memory[sf->memory[sf->pc + 1]] = (temp_ROL & 0xFF);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ACCUM:
                            temp_ROL = (uint16_t)sf->accumulator; 
                            temp_ROL = temp_ROL << 1;
                            // check for shift in a 1
                            if (sf->status & (1 << CARRY_INDEX)) {
                                temp_ROL += 1;
                            }
                            // check for shift into carry
                            if (temp_ROL & 0x100) {
                                sf->status |= (1 << CARRY_INDEX);
                            }
                            // check for negative/zero
                            if (temp_ROL & 0x80) {
                                sf->status |= (1 << NEGATIVE_INDEX);
                            } else if ((temp_ROL & 0xFF) == 0) {
                                sf->status |= (1 << ZERO_INDEX);
                            }
                            sf->accumulator = (temp_ROL & 0xFF);
                            sf->pc += 1;
                            break;
                        case ADDR_MODE_ABS:
                            temp_ROL = (uint16_t)sf->memory[(sf->memory[sf->pc + 2] << 8) | sf->memory[sf->pc + 1]];
                            temp_ROL = temp_ROL << 1;
                            // check for shift in a 1
                            if (sf->status & (1 << CARRY_INDEX)) {
                                temp_ROL += 1;
                            }
                            // check for shift into carry
                            if (temp_ROL & 0x100) {
                                sf->status |= (1 << CARRY_INDEX);
                            }
                            // check for negative/zero
                            if (temp_ROL & 0x80) {
                                sf->status |= (1 << NEGATIVE_INDEX);
                            } else if ((temp_ROL & 0xFF) == 0) {
                                sf->status |= (1 << ZERO_INDEX);
                            }
                            if (sf->memory[(sf->memory[sf->pc + 2] << 8) | sf->memory[sf->pc + 1]] & 0x80) {
                                sf->status |= (1 << CARRY_INDEX);
                            }
                            sf->memory[(sf->memory[sf->pc + 2] << 8) | sf->memory[sf->pc + 1]] = (temp_ROL & 0xFF);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ZPG_X:
                            temp_ROL = (uint16_t)sf->memory[sf->memory[sf->pc + 1] + sf->x_index]; 
                            temp_ROL = temp_ROL << 1;
                            // check for shift in a 1
                            if (sf->status & (1 << CARRY_INDEX)) {
                                temp_ROL += 1;
                            }
                            // check for shift into carry
                            if (temp_ROL & 0x100) {
                                sf->status |= (1 << CARRY_INDEX);
                            }
                            // check for negative/zero
                            if (temp_ROL & 0x80) {
                                sf->status |= (1 << NEGATIVE_INDEX);
                            } else if ((temp_ROL & 0xFF) == 0) {
                                sf->status |= (1 << ZERO_INDEX);
                            }
                            sf->memory[sf->memory[sf->pc + 1] + sf->x_index] = (temp_ROL & 0xFF);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_X:
                            temp_ROL = (uint16_t)sf->memory[((sf->memory[sf->pc + 2] << 8) | sf->memory[sf->pc + 1]) + sf->x_index];
                            temp_ROL = temp_ROL << 1;
                            // check for shift in a 1
                            if (sf->status & (1 << CARRY_INDEX)) {
                                temp_ROL += 1;
                            }
                            // check for shift into carry
                            if (temp_ROL & 0x100) {
                                sf->status |= (1 << CARRY_INDEX);
                            }
                            // check for negative/zero
                            if (temp_ROL & 0x80) {
                                sf->status |= (1 << NEGATIVE_INDEX);
                            } else if ((temp_ROL & 0xFF) == 0) {
                                sf->status |= (1 << ZERO_INDEX);
                            }
                            if (sf->memory[(sf->memory[sf->pc + 2] << 8) | sf->memory[sf->pc + 1]] & 0x80) {
                                sf->status |= (1 << CARRY_INDEX);
                            }
                            sf->memory[((sf->memory[sf->pc + 2] << 8) | sf->memory[sf->pc + 1]) + sf->x_index] = (temp_ROL & 0xFF);
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    break;

                case ((OP_EOR & AAA_BITMASK) >> 3) | (OP_EOR & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            sf->accumulator ^= sf->memory[((sf->memory[((sf->x_index + sf->memory[sf->pc + 1]) + 1) % 0xFF] << 8) |
                                               (sf->memory[(sf->x_index + sf->memory[sf->pc + 1]) % 0xFF]))];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG:
                            sf->accumulator ^= sf->memory[sf->memory[sf->pc + 1]];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_IMM:
                            sf->accumulator ^= sf->memory[sf->pc + 1];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            sf->accumulator ^= sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]];
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_IND_Y:
                            sf->accumulator ^= sf->memory[((sf->memory[sf->memory[sf->pc + 1] + 1] << 8)|
                                               sf->memory[sf->memory[sf->pc + 1]]) + sf->y_index];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG_X:
                            sf->accumulator ^= sf->memory[sf->memory[sf->pc + 1] + sf->x_index];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_Y:
                            sf->accumulator ^= sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->y_index];
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ABS_X:
                            sf->accumulator ^= sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->x_index];
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    
                    // set flags
                    if (sf->accumulator & 0x80) {
                        sf->status |= (1 << NEGATIVE_INDEX);
                    } else if (sf->accumulator == 0) {
                        sf->status |= (1 << ZERO_INDEX);
                    }
                    break;

                case ((OP_LSR & AAA_BITMASK) >> 3) | (OP_LSR & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_ZPG:
                            if (sf->memory[sf->memory[sf->pc + 1]] % 2) {
                                sf->status |= (1 << CARRY_INDEX);
                            }
                            sf->memory[sf->memory[sf->pc + 1]] = sf->memory[sf->memory[sf->pc + 1]] >> 1;
                            if (sf->memory[sf->memory[sf->pc + 1]] & 0x80) {
                                sf->status |= (1 << NEGATIVE_INDEX);
                            } else if (sf->memory[sf->memory[sf->pc + 1]] == 0) {
                                sf->status |= (1 << ZERO_INDEX);
                            }
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ACCUM:
                            if (sf->accumulator % 2) {
                                sf->status |= (1 << CARRY_INDEX);
                            }
                            sf->accumulator = sf->accumulator >> 1;
                            if (sf->accumulator & 0x80) {
                                sf->status |= (1 << NEGATIVE_INDEX);
                            } else if (sf->accumulator == 0) {
                                sf->status |= (1 << ZERO_INDEX);
                            }
                            sf->pc += 1;
                            break;
                        case ADDR_MODE_ABS:
                            if (sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]] % 2) {
                                sf->status |= (1 << CARRY_INDEX);
                            }
                            sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]] = sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]] >> 1;
                            if (sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]] & 0x80) {
                                sf->status |= (1 << NEGATIVE_INDEX);
                            } else if (sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]] == 0) {
                                sf->status |= (1 << ZERO_INDEX);
                            }
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ZPG_X:
                            if (sf->memory[sf->memory[sf->pc + 1] + sf->x_index] % 2) {
                                sf->status |= (1 << CARRY_INDEX);
                            }
                            sf->memory[sf->memory[sf->pc + 1] + sf->x_index] = sf->memory[sf->memory[sf->pc + 1] + sf->x_index] >> 1;
                            if (sf->memory[sf->memory[sf->pc + 1] + sf->x_index] & 0x80) {
                                sf->status |= (1 << NEGATIVE_INDEX);
                            } else if (sf->memory[sf->memory[sf->pc + 1] + sf->x_index] == 0) {
                                sf->status |= (1 << ZERO_INDEX);
                            }
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_X:
                            if (sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->x_index] % 2) {
                                sf->status |= (1 << CARRY_INDEX);
                            }
                            sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->x_index] = sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->x_index] >> 1;
                            if (sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->x_index] & 0x80) {
                                sf->status |= (1 << NEGATIVE_INDEX);
                            } else if (sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->x_index] == 0) {
                                sf->status |= (1 << ZERO_INDEX);
                            }
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    // logical shift so 0 will always be MSB
                    sf->status &= ~(1 << NEGATIVE_INDEX);
                    break;

                case ((OP_ADC & AAA_BITMASK) >> 3) | (OP_ADC & CC_BITMASK):
                    uint16_t temp_ADC;
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            temp_ADC = sf->accumulator + ((sf->status & (1 << CARRY_INDEX)) << 8) +
                                    sf->memory[(sf->memory[((sf->x_index + sf->memory[sf->pc + 1]) + 1) % 0xFF] << 8) |
                                    (sf->memory[(sf->x_index + sf->memory[sf->pc + 1]) % 0xFF])];
                            sf->accumulator = (temp_ADC & 0xFF);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG:
                            temp_ADC = sf->accumulator + ((sf->status & (1 << CARRY_INDEX)) << 8) +
                                    sf->memory[sf->memory[sf->pc + 1]];
                            sf->accumulator = (temp_ADC & 0xFF);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_IMM:
                            temp_ADC = sf->accumulator + ((sf->status & (1 << CARRY_INDEX)) << 8) +
                                    sf->memory[sf->pc + 1];
                            sf->accumulator = (temp_ADC & 0xFF);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            temp_ADC = sf->accumulator + ((sf->status & (1 << CARRY_INDEX)) << 8) +
                                    sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]];
                            sf->accumulator = (temp_ADC & 0xFF);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_IND_Y:
                            temp_ADC = sf->accumulator + ((sf->status & (1 << CARRY_INDEX)) << 8) +
                                    sf->memory[((sf->memory[sf->memory[sf->pc + 1] + 1] << 8)|
                                               sf->memory[sf->memory[sf->pc + 1]]) + sf->y_index];
                            sf->accumulator = (temp_ADC & 0xFF);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG_X:
                            temp_ADC = sf->accumulator + ((sf->status & (1 << CARRY_INDEX)) << 8) +
                                    sf->memory[sf->memory[sf->pc + 1] + sf->x_index];
                            sf->accumulator = (temp_ADC & 0xFF);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_Y:
                            temp_ADC = sf->accumulator + ((sf->status & (1 << CARRY_INDEX)) << 8) +
                                    sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->y_index];
                            sf->accumulator = (temp_ADC & 0xFF);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ABS_X:
                            temp_ADC = sf->accumulator + ((sf->status & (1 << CARRY_INDEX)) << 8) +
                                    sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->x_index];
                            sf->accumulator = (temp_ADC & 0xFF);
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    if (temp_ADC & 0x100) {
                        sf->status |= (1 << CARRY_INDEX);
                    }
                    if (temp_ADC & 0xF00) {
                        sf->status |= (1 << OVERFLOW_INDEX);
                    }
                    if (sf->accumulator & 0x80) {
                        sf->status |= (1 << NEGATIVE_INDEX);
                    } else if (sf->accumulator == 0) {
                        sf->status |= (1 << ZERO_INDEX);
                    }
                    break;

                case ((OP_ROR & AAA_BITMASK) >> 3) | (OP_ROR & CC_BITMASK):
                    uint8_t temp_ROR;
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_ZPG:
                            temp_ROR = sf->memory[sf->memory[sf->pc + 1]] >> 1;
                            if (sf->status & (1 << CARRY_INDEX)) {
                                temp_ROR |= 0x80;
                            }
                            if (sf->memory[sf->memory[sf->pc + 1]] % 2) {
                                sf->status |= (1 << CARRY_INDEX);
                            } else {
                                sf->status &= ~(1 << CARRY_INDEX);
                            }
                            sf->memory[sf->memory[sf->pc + 1]] = temp_ROR;
                            
                            if (sf->memory[sf->memory[sf->pc + 1]] & 0x80) {
                                sf->status |= (1 << NEGATIVE_INDEX);
                            } else {
                                sf->status &= ~(1 << NEGATIVE_INDEX);
                            }

                            if (sf->memory[sf->memory[sf->pc + 1]] == 0x00) {
                                sf->status |= (1 << ZERO_INDEX);
                            } else {
                                sf->status &= ~(1 << ZERO_INDEX);
                            }
                            
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ACCUM:
                            temp_ROR = sf->accumulator >> 1; 
                            if (sf->status & (1 << CARRY_INDEX)) {
                                temp_ROR |= 0x80;
                            }
                            if (sf->accumulator % 2) {
                                sf->status |= (1 << CARRY_INDEX);
                            } else {
                                sf->status &= ~(1 << CARRY_INDEX);
                            }
                            sf->accumulator = temp_ROR;

                            if (sf->accumulator & 0x80) {
                                sf->status |= (1 << NEGATIVE_INDEX);
                            } else {
                                sf->status &= ~(1 << NEGATIVE_INDEX);
                            }

                            if (sf->accumulator == 0x00) {
                                sf->status |= (1 << ZERO_INDEX);
                            } else {
                                sf->status &= ~(1 << ZERO_INDEX);
                            }

                            sf->pc += 1;
                            break;
                        case ADDR_MODE_ABS:
                            temp_ROR = sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]] >> 1;
                            if (sf->status & (1 << CARRY_INDEX)) {
                                temp_ROR |= 0x80;
                            }
                            if (sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]] % 2) {
                                sf->status |= (1 << CARRY_INDEX);
                            } else {
                                sf->status &= ~(1 << CARRY_INDEX);
                            }
                            sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]] = temp_ROR;
                            
                            if (sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]] & 0x80) {
                                sf->status |= (1 << NEGATIVE_INDEX);
                            } else {
                                sf->status &= ~(1 << NEGATIVE_INDEX);
                            }

                            if (sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]] == 0x00) {
                                sf->status |= (1 << ZERO_INDEX);
                            } else {
                                sf->status &= ~(1 << ZERO_INDEX);
                            }

                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ZPG_X:
                            temp_ROR = sf->memory[sf->memory[sf->pc + 1] + sf->x_index] >> 1;
                            if (sf->status & (1 << CARRY_INDEX)) {
                                temp_ROR |= 0x80;
                            }
                            if (sf->memory[sf->memory[sf->pc + 1] + sf->x_index] % 2) {
                                sf->status |= (1 << CARRY_INDEX);
                            } else {
                                sf->status &= ~(1 << CARRY_INDEX);
                            }
                            sf->memory[sf->memory[sf->pc + 1] + sf->x_index] = temp_ROR;
                            
                            if (sf->memory[sf->memory[sf->pc + 1] + sf->x_index] & 0x80) {
                                sf->status |= (1 << NEGATIVE_INDEX);
                            } else {
                                sf->status &= ~(1 << NEGATIVE_INDEX);
                            }

                            if (sf->memory[sf->memory[sf->pc + 1] + sf->x_index] == 0x00) {
                                sf->status |= (1 << ZERO_INDEX);
                            } else {
                                sf->status &= ~(1 << ZERO_INDEX);
                            }
                            
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_X:
                            temp_ROR = sf->memory[((sf->memory[sf->pc + 2] << 8) | sf->memory[sf->pc + 1]) + sf->x_index] >> 1;
                            if (sf->status & (1 << CARRY_INDEX)) {
                                temp_ROR |= 0x80;
                            }
                            if (sf->memory[((sf->memory[sf->pc + 2] << 8) | sf->memory[sf->pc + 1]) + sf->x_index] % 2) {
                                sf->status |= (1 << CARRY_INDEX);
                            } else {
                                sf->status &= ~(1 << CARRY_INDEX);
                            }
                            sf->memory[((sf->memory[sf->pc + 2] << 8) | sf->memory[sf->pc + 1]) + sf->x_index] = temp_ROR;
                            
                            if (sf->memory[((sf->memory[sf->pc + 2] << 8) | sf->memory[sf->pc + 1]) + sf->x_index] & 0x80) {
                                sf->status |= (1 << NEGATIVE_INDEX);
                            } else {
                                sf->status &= ~(1 << NEGATIVE_INDEX);
                            }

                            if (sf->memory[((sf->memory[sf->pc + 2] << 8) | sf->memory[sf->pc + 1]) + sf->x_index] == 0x00) {
                                sf->status |= (1 << ZERO_INDEX);
                            } else {
                                sf->status &= ~(1 << ZERO_INDEX);
                            }
                            
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }

                    break;

                case ((OP_STY & AAA_BITMASK) >> 3)|(OP_STY & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_ZPG:
                            sf->memory[sf->memory[sf->pc + 1]] = sf->y_index;
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]] = sf->y_index;
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ZPG_X:
                            sf->memory[sf->memory[sf->pc + 1] + sf->x_index] = sf->y_index;
                            sf->pc += 2;
                            break;
                        default:
                            break;
                    }
                    break;

                case ((OP_STA & AAA_BITMASK) >> 3)|(OP_STA & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            sf->memory[(sf->memory[(sf->memory[sf->pc + 1] + sf->x_index + 1) % 0xFF] << 8) |
                                        sf->memory[(sf->memory[sf->pc + 1] + sf->x_index) % 0xFF]] = sf->accumulator;
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG:
                            sf->memory[sf->memory[sf->pc + 1]] = sf->accumulator;
                            sf->pc += 2; 
                            break;
                        case ADDR_MODE_ABS:
                            sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]] = sf->accumulator;
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_IND_Y:
                            sf->memory[((sf->memory[sf->memory[sf->pc + 1] + 1] << 8)|sf->memory[sf->memory[sf->pc + 1]]) + sf->y_index] = sf->accumulator;
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG_X:
                            sf->memory[sf->memory[sf->pc + 1] + sf->x_index] = sf->accumulator;
                            sf->pc += 2; 
                            break;
                        case ADDR_MODE_ABS_Y:
                            sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->y_index] = sf->accumulator;
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ABS_X:
                            sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->x_index] = sf->accumulator;
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    break;

                case ((OP_STX & AAA_BITMASK) >> 3)|(OP_STX & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_ZPG:
                            sf->memory[sf->memory[sf->pc + 1]] = sf->x_index;
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]] = sf->x_index;
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ZPG_Y:
                            sf->memory[sf->memory[sf->pc + 1] + sf->y_index] = sf->x_index;
                            sf->pc += 2;
                            break;
                        default:
                            break;
                    }
                    break;

                case ((OP_LDY & AAA_BITMASK) >> 3)|(OP_LDY & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_ZPG:
                            sf->y_index = sf->memory[sf->memory[sf->pc + 1]];
                            sf->pc += 2;
                            break;
                        case 0x00: // special case, this is immediate
                            sf->y_index = sf->memory[sf->pc + 1];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            sf->y_index = sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]];
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ZPG_X:
                            sf->y_index = sf->memory[sf->memory[sf->pc + 1] + sf->x_index];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_X:
                            sf->y_index = sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->x_index];
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    if (sf->y_index & 0x80) {
                        sf->status |= (1 << NEGATIVE_INDEX);
                    } else {
                        sf->status &= ~(1 << NEGATIVE_INDEX);
                    }

                    if (sf->y_index == 0x00) {
                        sf->status |= (1 << ZERO_INDEX);
                    } else {
                        sf->status &= ~(1 << ZERO_INDEX);
                    }
                    break;

                case ((OP_LDA & AAA_BITMASK) >> 3)|(OP_LDA & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            sf->accumulator = sf->memory[(sf->memory[((sf->x_index + sf->memory[sf->pc + 1]) + 1) % 0xFF] << 8) |
                                                (sf->memory[(sf->x_index + sf->memory[sf->pc + 1]) % 0xFF])];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG:
                            sf->accumulator = sf->memory[sf->memory[sf->pc + 1]];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_IMM:
                            sf->accumulator = sf->memory[sf->pc + 1];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            sf->accumulator = sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]];
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_IND_Y:
                            sf->accumulator =  sf->memory[((sf->memory[sf->memory[sf->pc + 1] + 1] << 8)|
                                               sf->memory[sf->memory[sf->pc + 1]]) + sf->y_index];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG_X:
                            sf->accumulator = sf->memory[sf->memory[sf->pc + 1] + sf->x_index];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_Y:
                            sf->accumulator = sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->y_index];
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ABS_X:
                            sf->accumulator = sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->x_index];
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    if (sf->accumulator & 0x80) {
                        sf->status |= (1 << NEGATIVE_INDEX);
                    } else {
                        sf->status &= ~(1 << NEGATIVE_INDEX);
                    }

                    if (sf->accumulator == 0x00) {
                        sf->status |= (1 << ZERO_INDEX);
                    } else {
                        sf->status &= ~(1 << ZERO_INDEX);
                    }

                    break;

                case ((OP_LDX & AAA_BITMASK) >> 3)|(OP_LDX & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_ZPG:
                            sf->x_index = sf->memory[sf->memory[sf->pc + 1]];
                            sf->pc += 2;
                            break;
                        case 0x00: // special case, this is immediate
                            sf->x_index = sf->memory[sf->pc + 1];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            sf->x_index = sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]];
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ZPG_Y:
                            sf->x_index = sf->memory[sf->memory[sf->pc + 1] + sf->y_index];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_X: // special case, we use y_index for indexing here
                            sf->x_index = sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->y_index];
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    
                    if (sf->x_index & 0x80) {
                        sf->status |= (1 << NEGATIVE_INDEX);
                    } else {
                        sf->status &= ~(1 << NEGATIVE_INDEX);
                    }

                    if (sf->x_index == 0x00) {
                        sf->status |= (1 << ZERO_INDEX);
                    } else {
                        sf->status &= ~(1 << ZERO_INDEX);
                    }
                    break;

                case ((OP_CPY & AAA_BITMASK) >> 3)|(OP_CPY & CC_BITMASK):
                    uint8_t temp_CPY;
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case 0x00: // special case: immediate
                            if (sf->y_index < sf->memory[sf->pc + 1]) {
                                sf->status |= (1 << CARRY_INDEX);
                            } else {
                                sf->status &= ~(1 << CARRY_INDEX);
                            }
                            temp_CPY = sf->y_index - sf->memory[sf->pc + 1];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG:
                            if (sf->y_index < sf->memory[sf->memory[sf->pc + 1]]) {
                                sf->status |= (1 << CARRY_INDEX);
                            } else {
                                sf->status &= ~(1 << CARRY_INDEX);
                            }
                            temp_CPY = sf->y_index - sf->memory[sf->memory[sf->pc + 1]];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            if (sf->y_index < sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]]) {
                                sf->status |= (1 << CARRY_INDEX);
                            } else {
                                sf->status &= ~(1 << CARRY_INDEX);
                            }
                            temp_CPY = sf->y_index - sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]];
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    
                    if (temp_CPY & 0x80) {
                        sf->status |= (1 << NEGATIVE_INDEX);
                    } else {
                        sf->status &= ~(1 << NEGATIVE_INDEX);
                    }

                    if (temp_CPY == 0x00) {
                        sf->status |= (1 << ZERO_INDEX);
                    } else {
                        sf->status &= ~(1 << ZERO_INDEX);
                    }

                    break;

                case ((OP_CMP & AAA_BITMASK) >> 3)|(OP_CMP & CC_BITMASK):
                    uint8_t temp_CMP;
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            if (sf->accumulator < sf->memory[(sf->memory[((sf->x_index + sf->memory[sf->pc + 1]) + 1) % 0xFF] << 8) |
                                                (sf->memory[(sf->x_index + sf->memory[sf->pc + 1]) % 0xFF])]) {
                                sf->status |= (1 << CARRY_INDEX);
                            } else {
                                sf->status &= ~(1 << CARRY_INDEX);
                            }
                            temp_CMP = sf->accumulator - sf->memory[(sf->memory[((sf->x_index + sf->memory[sf->pc + 1]) + 1) % 0xFF] << 8) |
                                                (sf->memory[(sf->x_index + sf->memory[sf->pc + 1]) % 0xFF])];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG:
                            if (sf->accumulator < sf->memory[sf->memory[sf->pc + 1]]) {
                                sf->status |= (1 << CARRY_INDEX);
                            } else {
                                sf->status &= ~(1 << CARRY_INDEX);
                            }
                            temp_CMP = sf->accumulator - sf->memory[sf->memory[sf->pc + 1]];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_IMM:
                            if (sf->accumulator < sf->memory[sf->pc + 1]) {
                                sf->status |= (1 << CARRY_INDEX);
                            } else {
                                sf->status &= ~(1 << CARRY_INDEX);
                            }
                            temp_CMP = sf->accumulator - sf->memory[sf->pc + 1];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            if (sf->accumulator < sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]]) {
                                sf->status |= (1 << CARRY_INDEX);
                            } else {
                                sf->status &= ~(1 << CARRY_INDEX);
                            }
                            temp_CMP = sf->accumulator - sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]];
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_IND_Y:
                            if (sf->accumulator < sf->memory[((sf->memory[sf->memory[sf->pc + 1] + 1] << 8)|
                                               sf->memory[sf->memory[sf->pc + 1]]) + sf->y_index]) {
                                sf->status |= (1 << CARRY_INDEX);
                            } else {
                                sf->status &= ~(1 << CARRY_INDEX);
                            }
                            temp_CMP = sf->accumulator - sf->memory[((sf->memory[sf->memory[sf->pc + 1] + 1] << 8)|
                                               sf->memory[sf->memory[sf->pc + 1]]) + sf->y_index];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG_X:
                            if (sf->accumulator < sf->memory[sf->memory[sf->pc + 1] + sf->x_index]) {
                                sf->status |= (1 << CARRY_INDEX);
                            } else {
                                sf->status &= ~(1 << CARRY_INDEX);
                            }
                            temp_CMP = sf->accumulator - sf->memory[sf->memory[sf->pc + 1] + sf->x_index];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_Y:
                            if (sf->accumulator < sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->y_index]) {
                                sf->status |= (1 << CARRY_INDEX);
                            } else {
                                sf->status &= ~(1 << CARRY_INDEX);
                            }
                            sf->accumulator = sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->y_index];
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ABS_X:
                            if (sf->accumulator < sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->x_index]) {
                                sf->status |= (1 << CARRY_INDEX);
                            } else {
                                sf->status &= ~(1 << CARRY_INDEX);
                            }
                            sf->accumulator = sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->x_index];
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }

                    if (temp_CMP & 0x80) {
                        sf->status |= (1 << NEGATIVE_INDEX);
                    } else {
                        sf->status &= ~(1 << NEGATIVE_INDEX);
                    }

                    if (temp_CMP == 0x00) {
                        sf->status |= (1 << ZERO_INDEX);
                    } else {
                        sf->status &= ~(1 << ZERO_INDEX);
                    }
                    break;

                case ((OP_DEC & AAA_BITMASK) >> 3)|(OP_DEC & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_ZPG:
                            sf->memory[sf->memory[sf->pc + 1]] -= 1;
                            
                            if (sf->memory[sf->memory[sf->pc + 1]] & 0x80) {
                                sf->status |= (1 << NEGATIVE_INDEX);
                            } else {
                                sf->status &= ~(1 << NEGATIVE_INDEX);
                            }

                            if (sf->memory[sf->memory[sf->pc + 1]] == 0x00) {
                                sf->status |= (1 << ZERO_INDEX);
                            } else {
                                sf->status &= ~(1 << ZERO_INDEX);
                            }
                            
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]] -= 1;

                            if (sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]] & 0x80) {
                                sf->status |= (1 << NEGATIVE_INDEX);
                            } else {
                                sf->status &= ~(1 << NEGATIVE_INDEX);
                            }

                            if (sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]] == 0x00) {
                                sf->status |= (1 << ZERO_INDEX);
                            } else {
                                sf->status &= ~(1 << ZERO_INDEX);
                            }

                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ZPG_X:
                            sf->memory[sf->memory[sf->pc + 1] + sf->x_index] -= 1;

                            if (sf->memory[sf->memory[sf->pc + 1] + sf->x_index] & 0x80) {
                                sf->status |= (1 << NEGATIVE_INDEX);
                            } else {
                                sf->status &= ~(1 << NEGATIVE_INDEX);
                            }

                            if (sf->memory[sf->memory[sf->pc + 1] + sf->x_index] == 0x00) {
                                sf->status |= (1 << ZERO_INDEX);
                            } else {
                                sf->status &= ~(1 << ZERO_INDEX);
                            }

                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_X:
                            sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->x_index] -= 1;

                            if (sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->x_index] & 0x80) {
                                sf->status |= (1 << NEGATIVE_INDEX);
                            } else {
                                sf->status &= ~(1 << NEGATIVE_INDEX);
                            }

                            if (sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->x_index] == 0x00) {
                                sf->status |= (1 << ZERO_INDEX);
                            } else {
                                sf->status &= ~(1 << ZERO_INDEX);
                            }

                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    break;

                case ((OP_CPX & AAA_BITMASK) >> 3)|(OP_CPX & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case 0x00: // special case: immediate
                            if (sf->x_index < sf->memory[sf->pc + 1]) {
                                sf->status |= (1 << CARRY_INDEX);
                            } else {
                                sf->status &= ~(1 << CARRY_INDEX);
                            }
                            temp_CPY = sf->x_index - sf->memory[sf->pc + 1];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG:
                            if (sf->x_index < sf->memory[sf->memory[sf->pc + 1]]) {
                                sf->status |= (1 << CARRY_INDEX);
                            } else {
                                sf->status &= ~(1 << CARRY_INDEX);
                            }
                            temp_CPY = sf->x_index - sf->memory[sf->memory[sf->pc + 1]];
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            if (sf->x_index < sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]]) {
                                sf->status |= (1 << CARRY_INDEX);
                            } else {
                                sf->status &= ~(1 << CARRY_INDEX);
                            }
                            temp_CPY = sf->x_index - sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]];
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    
                    if (temp_CPY & 0x80) {
                        sf->status |= (1 << NEGATIVE_INDEX);
                    } else {
                        sf->status &= ~(1 << NEGATIVE_INDEX);
                    }

                    if (temp_CPY == 0x00) {
                        sf->status |= (1 << ZERO_INDEX);
                    } else {
                        sf->status &= ~(1 << ZERO_INDEX);
                    }
                    break;

                case ((OP_SBC & AAA_BITMASK) >> 3)|(OP_SBC & CC_BITMASK):
                    uint8_t temp_SBC;
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            temp_SBC = sf->accumulator - (sf->memory[(sf->memory[((sf->x_index + sf->memory[sf->pc + 1]) + 1) % 0xFF] << 8) | (sf->memory[(sf->x_index + sf->memory[sf->pc + 1]) % 0xFF])] 
                                                         + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX));
                            
                            
                            // positive - negative overflow to negative
                            if (((temp_SBC & 0x80) && 
                                 (!(sf->accumulator & 0x80)) && 
                                 ((sf->memory[(((sf->memory[sf->pc + 1] + sf->x_index + 1) & 0xFF) << 8)|sf->memory[(sf->memory[sf->pc + 1] + sf->x_index) % 0xFF]] + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX)) & 0x80)) ||
                                 // negative - positive overflow to positive
                                 (!(temp_SBC & 0x80)) &&
                                 (sf->accumulator & 0x80) &&
                                 (!((((sf->memory[(((sf->memory[sf->pc + 1] + sf->x_index + 1) & 0xFF) << 8)|sf->memory[(sf->memory[sf->pc + 1] + sf->x_index) % 0xFF]] + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX)) & 0x80))))
                                 ) {
                                sf->status |= (1 << OVERFLOW_INDEX);
                            } else {
                                sf->status &= ~(1 << OVERFLOW_INDEX);
                            }

                            // check carry
                            if (sf->accumulator < (sf->memory[(((sf->memory[sf->pc + 1] + sf->x_index + 1) & 0xFF) << 8)|sf->memory[(sf->memory[sf->pc + 1] + sf->x_index) % 0xFF]]
                                                  + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX))) {
                                sf->status &= ~(1 << CARRY_INDEX);
                            } else {
                                sf->status |= (1 << CARRY_INDEX);
                            }

                            sf->accumulator = temp_SBC;
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG:
                            temp_SBC = sf->accumulator - (sf->memory[sf->memory[sf->pc + 1]] + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX));
                            
                            // positive - negative overflow to negative
                            if (((temp_SBC & 0x80) && 
                                 (!(sf->accumulator & 0x80)) && 
                                 ((sf->memory[sf->memory[sf->pc + 1]] + ((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX))) & 0x80)) ||
                                 // negative - positive overflow to positive
                                ((!(temp_SBC & 0x80)) &&
                                 (sf->accumulator & 0x80) &&
                                 (!((sf->memory[sf->memory[sf->pc + 1]] + ((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX))) & 0x80)))
                                 ) {
                                sf->status |= (1 << OVERFLOW_INDEX);
                            } else {
                                sf->status &= ~(1 << OVERFLOW_INDEX);
                            }

                            // check carry
                            if (sf->accumulator < (sf->memory[sf->memory[sf->pc + 1]] + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX))) {
                                sf->status &= ~(1 << CARRY_INDEX);
                            } else {
                                sf->status |= (1 << CARRY_INDEX);
                            }

                            sf->accumulator = temp_SBC;
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_IMM:
                            temp_SBC = sf->accumulator - (sf->memory[sf->pc + 1] + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX));
                            
                            // positive - negative overflow to negative
                            if (((temp_SBC & 0x80) && 
                                 (!(sf->accumulator & 0x80)) && 
                                 ((sf->memory[sf->pc + 1] + ((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX))) & 0x80)) ||
                                 // negative - positive overflow to positive
                                ((!(temp_SBC & 0x80)) &&
                                 (sf->accumulator & 0x80) &&
                                 (!((sf->memory[sf->pc + 1] + ((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX))) & 0x80)))
                                 ) {
                                sf->status |= (1 << OVERFLOW_INDEX);
                            } else {
                                sf->status &= ~(1 << OVERFLOW_INDEX);
                            }
                            
                            if (sf->accumulator < (sf->memory[sf->pc + 1] + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX))) {
                                sf->status &= ~(1 << CARRY_INDEX);
                            } else {
                                sf->status |= (1 << CARRY_INDEX);
                            }

                            sf->accumulator = temp_SBC;
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            temp_SBC = sf->accumulator - (sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]] + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX));
                            
                            // positive - negative overflow to negative
                            if (((temp_SBC & 0x80) && 
                                 (!(sf->accumulator & 0x80)) && 
                                 ((sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]] + ((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX))) & 0x80)) ||
                                 // negative - positive overflow to positive
                                ((!(temp_SBC & 0x80)) &&
                                 (sf->accumulator & 0x80) &&
                                 (!((sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]] + ((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX))) & 0x80)))
                                 ) {
                                sf->status |= (1 << OVERFLOW_INDEX);
                            } else {
                                sf->status &= ~(1 << OVERFLOW_INDEX);
                            }

                            if (sf->accumulator < (sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]] + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX))) {
                                sf->status &= ~(1 << CARRY_INDEX);
                            } else {
                                sf->status |= (1 << CARRY_INDEX);
                            }

                            sf->accumulator = temp_SBC;
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_IND_Y:
                            temp_SBC = sf->accumulator - (sf->memory[((sf->memory[sf->memory[sf->pc + 1] + 1] << 8)|sf->memory[sf->memory[sf->pc + 1]]) + sf->y_index] 
                                                            + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX));
                            
                            // positive - negative overflow to negative
                            if (((temp_SBC & 0x80) && 
                                 (!(sf->accumulator & 0x80)) && 
                                 ((sf->memory[((sf->memory[sf->memory[sf->pc + 1] + 1] << 8)|sf->memory[sf->memory[sf->pc + 1]]) + sf->y_index] + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX)) & 0x80)) ||
                                 // negative - positive overflow to positive
                                 (!(temp_SBC & 0x80)) &&
                                 (sf->accumulator & 0x80) &&
                                 (!((sf->memory[((sf->memory[sf->memory[sf->pc + 1] + 1] << 8)|sf->memory[sf->memory[sf->pc + 1]]) + sf->y_index] + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX)) & 0x80))
                                 ) {
                                sf->status |= (1 << OVERFLOW_INDEX);
                            } else {
                                sf->status &= ~(1 << OVERFLOW_INDEX);
                            }

                            if (sf->accumulator < (sf->memory[((sf->memory[sf->memory[sf->pc + 1] + 1] << 8)|sf->memory[sf->memory[sf->pc + 1]]) + sf->y_index] 
                                + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX))) {
                                sf->status &= ~(1 << CARRY_INDEX);
                            } else {
                                sf->status |= (1 << CARRY_INDEX);
                            }

                            sf->accumulator = temp_SBC;
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG_X:
                            temp_SBC = sf->accumulator - (sf->memory[sf->memory[sf->pc + 1] + sf->x_index] + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX));
                            
                            // positive - negative overflow to negative
                            if (((temp_SBC & 0x80) && 
                                 (!(sf->accumulator & 0x80)) && 
                                 ((sf->memory[sf->memory[sf->pc + 1] + sf->x_index] + ((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX))) & 0x80)) ||
                                 // negative - positive overflow to positive
                                ((!(temp_SBC & 0x80)) &&
                                 (sf->accumulator & 0x80) &&
                                 (!((sf->memory[sf->memory[sf->pc + 1] + sf->x_index] + ((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX))) & 0x80)))
                                 ) {
                                sf->status |= (1 << OVERFLOW_INDEX);
                            } else {
                                sf->status &= ~(1 << OVERFLOW_INDEX);
                            }

                            if (sf->accumulator < (sf->memory[sf->memory[sf->pc + 1] + sf->x_index] + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX))) {
                                sf->status &= ~(1 << CARRY_INDEX);
                            } else {
                                sf->status |= (1 << CARRY_INDEX);
                            }

                            sf->accumulator = temp_SBC;
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_Y:
                            temp_SBC = sf->accumulator - (sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->y_index] + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX));
                            
                            // positive - negative overflow to negative
                            if (((temp_SBC & 0x80) && 
                                 (!(sf->accumulator & 0x80)) && 
                                 ((sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->y_index] + ((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX))) & 0x80)) ||
                                 // negative - positive overflow to positive
                                ((!(temp_SBC & 0x80)) &&
                                 (sf->accumulator & 0x80) &&
                                 (!((sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->y_index] + ((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX))) & 0x80)))
                                 ) {
                                sf->status |= (1 << OVERFLOW_INDEX);
                            } else {
                                sf->status &= ~(1 << OVERFLOW_INDEX);
                            }

                            if (sf->accumulator < (sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->y_index] + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX))) {
                                sf->status &= ~(1 << CARRY_INDEX);
                            } else {
                                sf->status |= (1 << CARRY_INDEX);
                            }

                            sf->accumulator = temp_SBC;
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ABS_X:
                            temp_SBC = sf->accumulator - (sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->x_index] + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX));
                            
                            // positive - negative overflow to negative
                            if (((temp_SBC & 0x80) && 
                                 (!(sf->accumulator & 0x80)) && 
                                 ((sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->x_index] + ((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX))) & 0x80)) ||
                                 // negative - positive overflow to positive
                                ((!(temp_SBC & 0x80)) &&
                                 (sf->accumulator & 0x80) &&
                                 (!((sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->x_index] + ((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX))) & 0x80)))
                                 ) {
                                sf->status |= (1 << OVERFLOW_INDEX);
                            } else {
                                sf->status &= ~(1 << OVERFLOW_INDEX);
                            }

                            if (sf->accumulator < (sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->x_index] + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX))) {
                                sf->status &= ~(1 << CARRY_INDEX);
                            } else {
                                sf->status |= (1 << CARRY_INDEX);
                            }

                            sf->accumulator = temp_SBC;
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }

                    if (sf->accumulator & 0x80) {
                        sf->status |= (1 << NEGATIVE_INDEX);
                    } else {
                        sf->status &= ~(1 << NEGATIVE_INDEX);
                    }

                    if (sf->accumulator == 0x00) {
                        sf->status |= (1 << ZERO_INDEX);
                    } else {
                        sf->status &= ~(1 << ZERO_INDEX);
                    }

                    break;

                case ((OP_INC & AAA_BITMASK) >> 3)|(OP_INC & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_ZPG:
                            sf->memory[sf->memory[sf->pc + 1]] += 1;
                            
                            if (sf->memory[sf->memory[sf->pc + 1]] & 0x80) {
                                sf->status |= (1 << NEGATIVE_INDEX);
                            } else {
                                sf->status &= ~(1 << NEGATIVE_INDEX);
                            }

                            if (sf->memory[sf->memory[sf->pc + 1]] == 0x00) {
                                sf->status |= (1 << ZERO_INDEX);
                            } else {
                                sf->status &= ~(1 << ZERO_INDEX);
                            }
                            
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]] += 1;

                            if (sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]] & 0x80) {
                                sf->status |= (1 << NEGATIVE_INDEX);
                            } else {
                                sf->status &= ~(1 << NEGATIVE_INDEX);
                            }

                            if (sf->memory[(sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]] == 0x00) {
                                sf->status |= (1 << ZERO_INDEX);
                            } else {
                                sf->status &= ~(1 << ZERO_INDEX);
                            }

                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ZPG_X:
                            sf->memory[sf->memory[sf->pc + 1] + sf->x_index] += 1;

                            if (sf->memory[sf->memory[sf->pc + 1] + sf->x_index] & 0x80) {
                                sf->status |= (1 << NEGATIVE_INDEX);
                            } else {
                                sf->status &= ~(1 << NEGATIVE_INDEX);
                            }

                            if (sf->memory[sf->memory[sf->pc + 1] + sf->x_index] == 0x00) {
                                sf->status |= (1 << ZERO_INDEX);
                            } else {
                                sf->status &= ~(1 << ZERO_INDEX);
                            }

                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_X:
                            sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->x_index] += 1;

                            if (sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->x_index] & 0x80) {
                                sf->status |= (1 << NEGATIVE_INDEX);
                            } else {
                                sf->status &= ~(1 << NEGATIVE_INDEX);
                            }

                            if (sf->memory[((sf->memory[sf->pc + 2] << 8)|sf->memory[sf->pc + 1]) + sf->x_index] == 0x00) {
                                sf->status |= (1 << ZERO_INDEX);
                            } else {
                                sf->status &= ~(1 << ZERO_INDEX);
                            }

                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    break;
            }
            break;

    }
}
