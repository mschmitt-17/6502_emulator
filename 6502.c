#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lib.h"
#include "6502.h"

void load_bytecode(sf_t *sf, Bytecode_t *bc, uint16_t load_address, uint32_t num_bytes) {
    if(load_address + num_bytes > MEMORY_SIZE) {
        fprintf(stderr, "Insufficient memory to load bytecode at address %u\n", load_address);
        exit(ERR_NO_MEM);
    }
    memcpy(sf->memory + sf->pc, bc->start, num_bytes);
}

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

static void check_negative_and_zero(sf_t *sf, uint8_t operand) {
    if (operand & 0x80) {
        sf->status |= (1 << NEGATIVE_INDEX);
    } else {
        sf->status &= ~(1 << NEGATIVE_INDEX);
    }

    if (operand == 0x00) {
        sf->status |= (1 << ZERO_INDEX);
    } else {
        sf->status &= ~(1 << ZERO_INDEX);
    }
}

static uint8_t hex_to_bcd(uint8_t hex) {
    if (((hex & 0xF0) > 0x90) || ((hex & 0x0F) > 0x09)) { // if we have A-F in number we wish to convert, return error
        return 0xFF;
    }
    uint8_t dec = ((hex & 0xF0) >> 4) * 10;
    dec += hex & 0x0F;
    return dec;
}

static uint8_t bcd_to_hex(uint8_t dec) {
    if (dec > 99) {
        return 0xFF;
    }
    uint8_t hex = (dec/10) * 16;
    hex += dec % 10;
    return hex;
}

static void ORA_operation(sf_t *sf, uint8_t *operand) {
    sf->accumulator |= *operand;
    check_negative_and_zero(sf, sf->accumulator);
}

static void ASL_operation(sf_t *sf, uint8_t *operand) {
    if ((*operand) & 0x80) {
        sf->status |= (1 << CARRY_INDEX);
    } else {
        sf->status &= ~(1 << CARRY_INDEX);
    }
    *operand = (*operand) << 1;
    check_negative_and_zero(sf, *operand);
}

static void BIT_operation(sf_t *sf, uint8_t *operand) {
    if ((sf->accumulator & (*operand)) == 0x00) {
        sf->status |= (1 << ZERO_INDEX);
    } else {
        sf->status &= ~(1 << ZERO_INDEX);
    }
    sf->status |= (sf->accumulator & (*operand)) & (1 << NEGATIVE_INDEX);
    sf->status |= (sf->accumulator & (*operand)) & (1 << OVERFLOW_INDEX);
}

static void AND_operation(sf_t *sf, uint8_t *operand) {
    sf->accumulator &= *operand;
    check_negative_and_zero(sf, sf->accumulator);
}

static void ROL_operation(sf_t *sf, uint8_t *operand) {
    uint16_t temp = (uint16_t)(*operand); 
    temp = temp << 1;
    
    // check for shift in a 1
    if (sf->status & (1 << CARRY_INDEX)) {
        temp += 1;
    }
    // check for shift into carry
    if (temp & 0x100) {
        sf->status |= (1 << CARRY_INDEX);
    } else {
        sf->status &= ~(1 << CARRY_INDEX);
    }
    
    *operand = (temp & 0xFF);
    check_negative_and_zero(sf, *operand);
}

static void EOR_operation(sf_t *sf, uint8_t *operand) {
    sf->accumulator ^= *operand;
    check_negative_and_zero(sf, sf->accumulator);
}

static void LSR_operation(sf_t *sf, uint8_t *operand) {
    if ((*operand) % 2) {
        sf->status |= (1 << CARRY_INDEX);
    } else {
        sf->status &= ~(1 << CARRY_INDEX);
    }
    *operand = (*operand) >> 1;
    check_negative_and_zero(sf, *operand);
}

static void ADC_operation(sf_t *sf, uint8_t *operand) {
    if (sf->status & (1 << DECIMAL_INDEX)) {
        uint8_t temp = hex_to_bcd(sf->accumulator) + hex_to_bcd(*operand) + ((sf->status & (1 << CARRY_INDEX)) >> (CARRY_INDEX));
        
        if (temp > 99) {
            sf->status |= (1 << CARRY_INDEX);
        } else {
            sf->status &= ~(1 << CARRY_INDEX);
        }

        temp = temp % 100;

        if (((temp > 80) && // positive + positive = negative
             (hex_to_bcd(sf->accumulator) < 80) &&
             ((hex_to_bcd(*operand) + ((sf->status & (1 << CARRY_INDEX)) >> (CARRY_INDEX))) < 80)) ||
           ((temp < 80) && // negative + negative = positive
            (hex_to_bcd(sf->accumulator) > 80) &&
            ((hex_to_bcd(*operand) + ((sf->status & (1 << CARRY_INDEX)) >> (CARRY_INDEX))) > 80))) {
            sf->status |= (1 << OVERFLOW_INDEX);
        } else {
            sf->status &= ~(1 << OVERFLOW_INDEX);
        }

        sf->accumulator = bcd_to_hex(temp);
    } else {
        uint16_t temp = sf->accumulator + ((sf->status & (1 << CARRY_INDEX)) >> (CARRY_INDEX)) + *operand;
        if (((temp & 0x80) && // positive + positive = negative
            (!(sf->accumulator & 0x80)) &&
            (!(((*operand) + ((sf->status & (1 << CARRY_INDEX)) >> (CARRY_INDEX))) & 0x80))) ||
            ((!(temp & 0x80)) && // negative + negative = positive
            (sf->accumulator & 0x80) &&
            (((*operand) + ((sf->status & (1 << CARRY_INDEX)) >> (CARRY_INDEX))) & 0x80))) {
            sf->status |= (1 << OVERFLOW_INDEX);
        } else {
            sf->status &= ~(1 << OVERFLOW_INDEX);
        }
        sf->accumulator = (temp & 0xFF);
        
        if (temp & 0x100) {
            sf->status |= (1 << CARRY_INDEX);
        } else {
            sf->status &= ~(1 << CARRY_INDEX);
        }
    }
    check_negative_and_zero(sf, sf->accumulator);
}

static void ROR_operation(sf_t *sf, uint8_t *operand) {
    uint8_t temp = (*operand) >> 1;
    if (sf->status & (1 << CARRY_INDEX)) {
        temp |= 0x80;
    }
    if ((*operand) % 2) {
        sf->status |= (1 << CARRY_INDEX);
    } else {
        sf->status &= ~(1 << CARRY_INDEX);
    }
    *operand = temp;
    
    if ((*operand) & 0x80) {
        sf->status |= (1 << NEGATIVE_INDEX);
    } else {
        sf->status &= ~(1 << NEGATIVE_INDEX);
    }

    if ((*operand) == 0x00) {
        sf->status |= (1 << ZERO_INDEX);
    } else {
        sf->status &= ~(1 << ZERO_INDEX);
    }
}

static void STY_operation(sf_t *sf, uint8_t *operand) {
    *operand = sf->y_index;
}

static void STA_operation(sf_t *sf, uint8_t *operand) {
    *operand = sf->accumulator;
}

static void STX_operation(sf_t *sf, uint8_t *operand) {
    *operand = sf->x_index;
}

static void LDY_operation(sf_t *sf, uint8_t *operand) {
    sf->y_index = *operand;
    
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
}

static void LDA_operation(sf_t *sf, uint8_t *operand) {
    sf->accumulator = *operand;
    
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
}

static void LDX_operation(sf_t *sf, uint8_t *operand) {
    sf->x_index = *operand;
    
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
}

static void CPY_operation(sf_t *sf, uint8_t *operand) {
    if (sf->y_index < *operand) {
        sf->status |= (1 << CARRY_INDEX);
    } else {
        sf->status &= ~(1 << CARRY_INDEX);
    }
    uint8_t temp = sf->y_index - *operand;

    if (temp & 0x80) {
        sf->status |= (1 << NEGATIVE_INDEX);
    } else {
        sf->status &= ~(1 << NEGATIVE_INDEX);
    }

    if (temp == 0x00) {
        sf->status |= (1 << ZERO_INDEX);
    } else {
        sf->status &= ~(1 << ZERO_INDEX);
    }
}

static void CMP_operation(sf_t *sf, uint8_t *operand) {
    if (sf->accumulator < *operand) {
        sf->status |= (1 << CARRY_INDEX);
    } else {
        sf->status &= ~(1 << CARRY_INDEX);
    }
    uint8_t temp = sf->accumulator - *operand;

    if (temp & 0x80) {
        sf->status |= (1 << NEGATIVE_INDEX);
    } else {
        sf->status &= ~(1 << NEGATIVE_INDEX);
    }

    if (temp == 0x00) {
        sf->status |= (1 << ZERO_INDEX);
    } else {
        sf->status &= ~(1 << ZERO_INDEX);
    }
}

static void DEC_operation(sf_t *sf, uint8_t *operand) {
    *operand -= 1;
                            
    if ((*operand) & 0x80) {
        sf->status |= (1 << NEGATIVE_INDEX);
    } else {
        sf->status &= ~(1 << NEGATIVE_INDEX);
    }

    if ((*operand) == 0x00) {
        sf->status |= (1 << ZERO_INDEX);
    } else {
        sf->status &= ~(1 << ZERO_INDEX);
    }
}

static void CPX_operation(sf_t *sf, uint8_t *operand) {
    if (sf->x_index < *operand) {
        sf->status |= (1 << CARRY_INDEX);
    } else {
        sf->status &= ~(1 << CARRY_INDEX);
    }
    uint8_t temp = sf->x_index - *operand;

    if (temp & 0x80) {
        sf->status |= (1 << NEGATIVE_INDEX);
    } else {
        sf->status &= ~(1 << NEGATIVE_INDEX);
    }

    if (temp == 0x00) {
        sf->status |= (1 << ZERO_INDEX);
    } else {
        sf->status &= ~(1 << ZERO_INDEX);
    }
}

static void SBC_operation(sf_t *sf, uint8_t *operand) {
    if (sf->status & (1 << DECIMAL_INDEX)) {
        uint8_t temp;
        if (hex_to_bcd(sf->accumulator) < (hex_to_bcd(*operand) + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX))) {
            temp = 99 - ((hex_to_bcd(*operand) + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX)) - hex_to_bcd(sf->accumulator));
            sf->status &= ~(1 << CARRY_INDEX);
        } else {
            temp = hex_to_bcd(sf->accumulator) - (hex_to_bcd(*operand) + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX));
            sf->status |= (1 << CARRY_INDEX);
        }

        if (((temp > 80) && 
            (hex_to_bcd(sf->accumulator) < 80) && 
            ((hex_to_bcd(*operand) + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX)) > 80)) ||
            ((temp < 80) &&
             (hex_to_bcd(sf->accumulator) > 80) &&
            (((hex_to_bcd(*operand) + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX)) < 80)))) {
            sf->status |= (1 << OVERFLOW_INDEX);
        } else {
            sf->status &= ~(1 << OVERFLOW_INDEX);
        }

        sf->accumulator = bcd_to_hex(temp);
    } else {
        uint8_t temp = sf->accumulator - ((*operand) + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX));
        
        if (((temp & 0x80) && 
            (!(sf->accumulator & 0x80)) && 
            (((*operand) + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX)) & 0x80)) ||
            (!(temp & 0x80)) &&
            (sf->accumulator & 0x80) &&
            (!(((((*operand) + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX)) & 0x80))))
            ) {
            sf->status |= (1 << OVERFLOW_INDEX);
        } else {
            sf->status &= ~(1 << OVERFLOW_INDEX);
        }

        // check carry
        if (sf->accumulator < ((*operand) + (((sf->status & (1 << CARRY_INDEX)) ^ (1 << CARRY_INDEX)) >> CARRY_INDEX))) {
            sf->status &= ~(1 << CARRY_INDEX);
        } else {
            sf->status |= (1 << CARRY_INDEX);
        }

        sf->accumulator = temp;
    }
    check_negative_and_zero(sf, sf->accumulator);
}

static void INC_operation(sf_t *sf, uint8_t *operand) {
    *operand += 1;

    if ((*operand) & 0x80) {
        sf->status |= (1 << NEGATIVE_INDEX);
    } else {
        sf->status &= ~(1 << NEGATIVE_INDEX);
    }

    if ((*operand) == 0x00) {
        sf->status |= (1 << ZERO_INDEX);
    } else {
        sf->status &= ~(1 << ZERO_INDEX);
    }
}

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
                sf->pc += (int8_t) sf->memory[sf->pc + 1]; // offset is stored in following byte
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
                sf->pc += (int8_t)sf->memory[sf->pc + 1];
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
                sf->pc += (int8_t)sf->memory[sf->pc + 1];
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
                sf->pc += (int8_t)sf->memory[sf->pc + 1];
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
            check_negative_and_zero(sf, sf->y_index);
            sf->pc++;
            break;

        case OP_TXA:
            sf->accumulator = sf->x_index;
            check_negative_and_zero(sf, sf->accumulator);
            sf->pc++;
            break;

        case OP_BCC:
            if (!(sf->status & (1 << CARRY_INDEX))) {
                sf->pc += (int8_t)sf->memory[sf->pc + 1];
            } else {
                sf->pc += 2;
            }
            break;

        case OP_TYA:
            sf->accumulator = sf->y_index;
            check_negative_and_zero(sf, sf->accumulator);
            sf->pc++;
            break;

        case OP_TXS:
            sf->esp = sf->x_index;
            sf->pc++;
            break;

        case OP_TAY:
            sf->y_index = sf->accumulator;
            check_negative_and_zero(sf, sf->y_index);
            sf->pc++;
            break;

        case OP_TAX:
            sf->x_index = sf->accumulator;
            check_negative_and_zero(sf, sf->x_index);
            sf->pc++;
            break;

        case OP_BCS:
            if (sf->status & (1 << CARRY_INDEX)) {
                sf->pc += (int8_t)sf->memory[sf->pc + 1];
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
            check_negative_and_zero(sf, sf->x_index);
            sf->pc++;
            break;

        case OP_INY:
            sf->y_index++;
            check_negative_and_zero(sf, sf->y_index);
            sf->pc++;
            break;

        case OP_DEX:
            sf->x_index--;
            check_negative_and_zero(sf, sf->x_index);
            sf->pc++;
            break;

        case OP_BNE:
            if (!(sf->status & (1 << ZERO_INDEX))) {
                sf->pc += (int8_t)sf->memory[sf->pc + 1];
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
            check_negative_and_zero(sf, sf->x_index);
            sf->pc++;
            break;

        case OP_NOP:
            sf->pc++;
            break;

        case OP_BEQ:
            if (sf->status & (1 << ZERO_INDEX)) {
                sf->pc += (int8_t)sf->memory[sf->pc + 1];
            } else {
                sf->pc += 2;
            }
            break;

        case OP_SED:
            sf->status |= (1 << DECIMAL_INDEX);
            sf->pc++;
            break;

        default:
            // non special-case operations
            switch (((sf->memory[sf->pc] & AAA_BITMASK) >> 3) | (sf->memory[sf->pc] & CC_BITMASK)) {
                case ((OP_ORA & AAA_BITMASK) >> 3) | (OP_ORA & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            ORA_operation(sf, &IND_X_MEM_ACCESS);                 
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG:
                            ORA_operation(sf, &ZPG_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_IMM:
                            ORA_operation(sf, &IMM_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            ORA_operation(sf, &ABS_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_IND_Y:
                            ORA_operation(sf, &IND_Y_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG_X:
                            ORA_operation(sf, &ZPG_X_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_Y:
                            ORA_operation(sf, &ABS_Y_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ABS_X:
                            ORA_operation(sf, &ABS_X_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    check_negative_and_zero(sf, sf->accumulator);
                    break;

                case ((OP_ASL & AAA_BITMASK) >> 3) | (OP_ASL & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_ZPG:
                            ASL_operation(sf, &ZPG_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ACCUM:
                            ASL_operation(sf, &sf->accumulator);
                            sf->pc += 1;
                            break;
                        case ADDR_MODE_ABS:
                            ASL_operation(sf, &ABS_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ZPG_X:
                            ASL_operation(sf, &ZPG_X_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_X:
                            ASL_operation(sf, &ABS_X_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    break;

                case ((OP_BIT & AAA_BITMASK) >> 3) | (OP_BIT & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_ZPG:
                            BIT_operation(sf, &ZPG_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            BIT_operation(sf, &ABS_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    break;

                case ((OP_AND & AAA_BITMASK) >> 3) | (OP_AND & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            AND_operation(sf, &IND_X_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG:
                            AND_operation(sf, &ZPG_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_IMM:
                            AND_operation(sf, &IMM_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            AND_operation(sf, &ABS_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_IND_Y:
                            AND_operation(sf, &IND_Y_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG_X:
                            AND_operation(sf, &ZPG_X_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_Y:
                            AND_operation(sf, &ABS_Y_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ABS_X:
                            AND_operation(sf, &ABS_X_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    break;

                case ((OP_ROL & AAA_BITMASK) >> 3) | (OP_ROL & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_ZPG:
                            ROL_operation(sf, &ZPG_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ACCUM:
                            ROL_operation(sf, &sf->accumulator);
                            sf->pc += 1;
                            break;
                        case ADDR_MODE_ABS:
                            ROL_operation(sf, &ABS_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ZPG_X:
                            ROL_operation(sf, &ZPG_X_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_X:
                            ROL_operation(sf, &ABS_X_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    break;

                case ((OP_EOR & AAA_BITMASK) >> 3) | (OP_EOR & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            EOR_operation(sf, &IND_X_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG:
                            EOR_operation(sf, &ZPG_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_IMM:
                            EOR_operation(sf, &IMM_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            EOR_operation(sf, &ABS_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_IND_Y:
                            EOR_operation(sf, &IND_Y_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG_X:
                            EOR_operation(sf, &ZPG_X_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_Y:
                            EOR_operation(sf, &ABS_Y_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ABS_X:
                            EOR_operation(sf, &ABS_X_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    break;

                case ((OP_LSR & AAA_BITMASK) >> 3) | (OP_LSR & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_ZPG:
                            LSR_operation(sf, &ZPG_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ACCUM:
                            LSR_operation(sf, &sf->accumulator);
                            sf->pc += 1;
                            break;
                        case ADDR_MODE_ABS:
                            LSR_operation(sf, &ABS_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ZPG_X:
                            LSR_operation(sf, &ZPG_X_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_X:
                            LSR_operation(sf, &ABS_X_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    break;

                case ((OP_ADC & AAA_BITMASK) >> 3) | (OP_ADC & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            ADC_operation(sf, &IND_X_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG:
                            ADC_operation(sf, &ZPG_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_IMM:
                            ADC_operation(sf, &IMM_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            ADC_operation(sf, &ABS_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_IND_Y:
                            ADC_operation(sf, &IND_Y_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG_X:
                            ADC_operation(sf, &ZPG_X_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_Y:
                            ADC_operation(sf, &ABS_Y_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ABS_X:
                            ADC_operation(sf, &ABS_X_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    break;

                case ((OP_ROR & AAA_BITMASK) >> 3) | (OP_ROR & CC_BITMASK):
                    uint8_t temp_ROR;
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_ZPG:
                            ROR_operation(sf, &ZPG_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ACCUM:
                            ROR_operation(sf, &sf->accumulator);
                            sf->pc += 1;
                            break;
                        case ADDR_MODE_ABS:
                            ROR_operation(sf, &ABS_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ZPG_X:
                            ROR_operation(sf, &ZPG_X_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_X:
                            ROR_operation(sf, &ABS_X_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    break;

                case ((OP_STY & AAA_BITMASK) >> 3)|(OP_STY & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_ZPG:
                            STY_operation(sf, &ZPG_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            STY_operation(sf, &ABS_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ZPG_X:
                            STY_operation(sf, &ZPG_X_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        default:
                            break;
                    }
                    break;

                case ((OP_STA & AAA_BITMASK) >> 3)|(OP_STA & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            STA_operation(sf, &IND_X_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG:
                            STA_operation(sf, &ZPG_MEM_ACCESS);
                            sf->pc += 2; 
                            break;
                        case ADDR_MODE_ABS:
                            STA_operation(sf, &ABS_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_IND_Y:
                            STA_operation(sf, &IND_Y_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG_X:
                            STA_operation(sf, &ZPG_X_MEM_ACCESS);
                            sf->pc += 2; 
                            break;
                        case ADDR_MODE_ABS_Y:
                            STA_operation(sf, &ABS_Y_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ABS_X:
                            STA_operation(sf, &ABS_X_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    break;

                case ((OP_STX & AAA_BITMASK) >> 3)|(OP_STX & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_ZPG:
                            STX_operation(sf, &ZPG_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            STX_operation(sf, &ABS_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ZPG_Y:
                            STX_operation(sf, &ZPG_Y_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        default:
                            break;
                    }
                    break;

                case ((OP_LDY & AAA_BITMASK) >> 3)|(OP_LDY & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_ZPG:
                            LDY_operation(sf, &ZPG_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case 0x00: // special case, this is immediate
                            LDY_operation(sf, &IMM_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            LDY_operation(sf, &ABS_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ZPG_X:
                            LDY_operation(sf, &ZPG_X_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_X:
                            LDY_operation(sf, &ABS_X_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    break;

                case ((OP_LDA & AAA_BITMASK) >> 3)|(OP_LDA & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            LDA_operation(sf, &IND_X_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG:
                            LDA_operation(sf, &ZPG_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_IMM:
                            LDA_operation(sf, &IMM_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            LDA_operation(sf, &ABS_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_IND_Y:
                            LDA_operation(sf, &IND_Y_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG_X:
                            LDA_operation(sf, &ZPG_X_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_Y:
                            LDA_operation(sf, &ABS_Y_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ABS_X:
                            LDA_operation(sf, &ABS_X_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    break;

                case ((OP_LDX & AAA_BITMASK) >> 3)|(OP_LDX & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_ZPG:
                            LDX_operation(sf, &ZPG_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case 0x00: // special case, this is immediate
                            LDX_operation(sf, &IMM_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            LDX_operation(sf, &ABS_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ZPG_Y:
                            LDX_operation(sf, &ZPG_Y_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_X: // special case, we use y_index for indexing here
                            LDX_operation(sf, &ABS_Y_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    break;

                case ((OP_CPY & AAA_BITMASK) >> 3)|(OP_CPY & CC_BITMASK):
                    uint8_t temp_CPY;
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case 0x00: // special case: immediate
                            CPY_operation(sf, &IMM_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG:
                            CPY_operation(sf, &ZPG_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            CPY_operation(sf, &ABS_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    break;

                case ((OP_CMP & AAA_BITMASK) >> 3)|(OP_CMP & CC_BITMASK):
                    uint8_t temp_CMP;
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            CMP_operation(sf, &IND_X_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG:
                            CMP_operation(sf, &ZPG_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_IMM:
                            CMP_operation(sf, &IMM_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            CMP_operation(sf, &ABS_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_IND_Y:
                            CMP_operation(sf, &IND_Y_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG_X:
                            CMP_operation(sf, &ZPG_X_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_Y:
                            CMP_operation(sf, &ABS_Y_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ABS_X:
                            CMP_operation(sf, &ABS_X_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    break;

                case ((OP_DEC & AAA_BITMASK) >> 3)|(OP_DEC & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_ZPG:
                            DEC_operation(sf, &ZPG_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            DEC_operation(sf, &ABS_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ZPG_X:
                            DEC_operation(sf, &ZPG_X_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_X:
                            DEC_operation(sf, &ABS_X_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    break;

                case ((OP_CPX & AAA_BITMASK) >> 3)|(OP_CPX & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case 0x00: // special case: immediate
                            CPX_operation(sf, &IMM_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG:
                            CPX_operation(sf, &ZPG_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            CPX_operation(sf, &ABS_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    break;

                case ((OP_SBC & AAA_BITMASK) >> 3)|(OP_SBC & CC_BITMASK):
                    uint8_t temp_SBC;
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_IND_X:
                            SBC_operation(sf, &IND_X_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG:
                            SBC_operation(sf, &ZPG_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_IMM:
                            SBC_operation(sf, &IMM_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            SBC_operation(sf, &ABS_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_IND_Y:
                            SBC_operation(sf, &IND_Y_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ZPG_X:
                            SBC_operation(sf, &ZPG_X_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_Y:
                            SBC_operation(sf, &ABS_Y_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ABS_X:
                            SBC_operation(sf, &ABS_X_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        default:
                            break;
                    }
                    break;

                case ((OP_INC & AAA_BITMASK) >> 3)|(OP_INC & CC_BITMASK):
                    switch((sf->memory[sf->pc] & BBB_BITMASK) >> 2) {
                        case ADDR_MODE_ZPG:
                            INC_operation(sf, &ZPG_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS:
                            INC_operation(sf, &ABS_MEM_ACCESS);
                            sf->pc += 3;
                            break;
                        case ADDR_MODE_ZPG_X:
                            INC_operation(sf, &ZPG_X_MEM_ACCESS);
                            sf->pc += 2;
                            break;
                        case ADDR_MODE_ABS_X:
                            INC_operation(sf, &ABS_X_MEM_ACCESS);
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
