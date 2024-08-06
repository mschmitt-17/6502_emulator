#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lib.h"
#include "6502.h"

/* load_bytecode
 *      DESCRIPTION: loads bytecode at passed address in memory
 *      INPUTS: sf -- pointer to 6502 struct with memory to use
 *              bc -- pointer to bytecode to load into memory
 *              load_address -- address to load bytecode at
 *              num_bytes -- number of bytes to load from start of bytecode
 *      OUTPUTS: none
 *      SIDE EFFECTS: fills 6502 memory with bytecode
 */
void load_bytecode(sf_t *sf, Bytecode_t *bc, uint16_t load_address, uint32_t num_bytes) {
    if(load_address + num_bytes > MEMORY_SIZE) {
        fprintf(stderr, "Insufficient memory to load bytecode at address %u\n", load_address);
        exit(ERR_NO_MEM);
    }
    memcpy(sf->memory + load_address, bc->start, num_bytes);
}

/* initialize_regs
 *      DESCRIPTION: sets pc to passed value, sets stack to start of stack, zeroes out other regs
 *      INPUTS: sf -- pointer to 6502 whose registers we wish to modify
 *              pc_init -- value to set pc to initially
 *      OUTPUTS: none
 *      SIDE EFFECTS: modifies register values
 */
void initialize_regs(sf_t *sf, uint16_t pc_init) {
    sf->esp = STACK_START; // stack grows down
    sf->pc = pc_init;
    sf->accumulator = 0;
    sf->x_index = 0;
    sf->y_index = 0;
    sf->status = 0;
}

/* check_negative_and_zero
 *      DESCRIPTION: checks if operand is negative or zero and sets flags accordingly
 *      INPUTS: sf -- 6502 whose flags we wish to modify
 *              operand -- operand whose negative or zero status we want to check
 *      OUTPUTS: none
 *      SIDE EFFECTS: modifies negative and zero flag of 6502
 */
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

/* hex_to_bcd
 *      DESCRIPTION: converts passed hex number to decimal WITHOUT PRESERVING VALUE, i.e. 0x99 -> 99
 *      INPUTS: hex -- the number to convert to decimal
 *      OUTPUTS: the converted number
 *      SIDE EFFECTS: none
 */
static uint8_t hex_to_bcd(uint8_t hex) {
    if (((hex & 0xF0) > 0x90) || ((hex & 0x0F) > 0x09)) { // if we have A-F in number we wish to convert, return error
        return 0xFF;
    }
    uint8_t dec = ((hex & 0xF0) >> 4) * 10;
    dec += hex & 0x0F;
    return dec;
}

/* bcd_to_hex
 *      DESCRIPTION: converts passed bcd number to hex WITHOUT PRESERVING VALUE, i.e. 99 -> 0x99
 *      INPUTS: dec -- the number to convert to hex
 *      OUTPUTS: the converted number
 *      SIDE EFFECTS: none
 */
static uint8_t bcd_to_hex(uint8_t dec) {
    if (dec > 99) {
        return 0xFF;
    }
    uint8_t hex = (dec/10) * 16;
    hex += dec % 10;
    return hex;
}

/* ORA_operation
 *      DESCRIPTION: ORs accumulator with operand and stores result in accumulator
 *      INPUTS: sf -- 6502 containing accumulator to OR
 *              operand -- operand to OR with
 *      OUTPUTS: none
 *      SIDE EFFECTS: modifies accumulator value, negative and zero flags
 */
static void ORA_operation(sf_t *sf, uint8_t *operand) {
    sf->accumulator |= *operand;
    check_negative_and_zero(sf, sf->accumulator);
}

/* ASL_operation
 *      DESCRIPTION: left shifts operand, setting carry flag if MSB is set
 *      INPUTS: sf -- 6502 struct
 *              operand -- operand to left shift
 *      OUTPUTS: none
 *      SIDE EFFECTS: left shifts operand, sets carry flag if MSB of operand is set, modifies negative and zero flags
 */
static void ASL_operation(sf_t *sf, uint8_t *operand) {
    if ((*operand) & 0x80) {
        sf->status |= (1 << CARRY_INDEX);
    } else {
        sf->status &= ~(1 << CARRY_INDEX);
    }
    *operand = (*operand) << 1;
    check_negative_and_zero(sf, *operand);
}

/* BIT_operation
 *      DESCRIPTION: ANDs accumulator with operand, setting zero, negative, and overflow flags
 *      INPUTS: sf -- 6502 struct
 *              operand -- operand to AND with accumulator
 *      OUTPUTS: none
 *      SIDE EFFECTS: sets zero flag if result of AND is zero, sets negative/overflow flag if result of and has bit 7/6 set
 */
static void BIT_operation(sf_t *sf, uint8_t *operand) {
    if ((sf->accumulator & (*operand)) == 0x00) {
        sf->status |= (1 << ZERO_INDEX);
    } else {
        sf->status &= ~(1 << ZERO_INDEX);
    }
    sf->status |= (sf->accumulator & (*operand)) & (1 << NEGATIVE_INDEX);
    sf->status |= (sf->accumulator & (*operand)) & (1 << OVERFLOW_INDEX);
}

/* AND_operation
 *      DESCRIPTION: ANDs accumulator with operand and stores result in accumulator
 *      INPUTS: sf -- 6502 struct
 *              operand -- operand to AND with accumulator
 *      OUTPUTS: none
 *      SIDE EFFECTS: modifies accumulator, negative and zero flags
 */
static void AND_operation(sf_t *sf, uint8_t *operand) {
    sf->accumulator &= *operand;
    check_negative_and_zero(sf, sf->accumulator);
}

/* ROL_operation
 *      DESCRIPTION: rotates passed operand left
 *      INPUTS: sf -- 6502 struct
 *              operand -- operand to rotate left
 *      OUTPUTS: none
 *      SIDE EFFECTS: sets carry flag if MSB of operand is positive, modifies negative and zero flags
 */
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

/* EOR_operation
 *      DESCRIPTION: XORs accumulator with operand, storing result in accumulator
 *      INPUTS: sf -- 6502 struct
 *              operand -- operand to XOR with accumulator
 *      OUTPUTS: none
 *      SIDE EFFECTS: modifies accumulator, negative and zero flags
 */
static void EOR_operation(sf_t *sf, uint8_t *operand) {
    sf->accumulator ^= *operand;
    check_negative_and_zero(sf, sf->accumulator);
}

/* LSR_operation
 *      DESCRIPTION: logical shifts operand right
 *      INPUTS: sf -- 6502 struct
 *              operand -- operand to logical shift
 *      OUTPUTS: none
 *      SIDE EFFECTS: sets carry flag if LSB of operand is set, modifies negative and zero flags
 */
static void LSR_operation(sf_t *sf, uint8_t *operand) {
    if ((*operand) % 2) {
        sf->status |= (1 << CARRY_INDEX);
    } else {
        sf->status &= ~(1 << CARRY_INDEX);
    }
    *operand = (*operand) >> 1;
    check_negative_and_zero(sf, *operand);
}

/* ADC_operation
 *      DESCRIPTION: adds operand to accumulator with carry, storing result in accumulator; works with BCD and hex
 *      INPUTS: sf -- 6502 struct
 *              operand -- operand to add to accumulator
 *      OUTPUTS: none
 *      SIDE EFFECTS: modifies accumulator; overflow, negative, zero and carry flags
 */
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

/* ROR_operation
 *      DESCRIPTION: rotates operand right
 *      INPUTS: sf -- 6502 struct
 *              operand -- operand to rotate right
 *      OUTPUTS: none
 *      SIDE EFFECTS: sets carry flag if LSB of operand is set, modifies negative and zero flags
 */
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

    check_negative_and_zero(sf, *operand);
}

/* STY_operation
 *      DESCRIPTION: stores y_index in memory
 *      INPUTS: sf -- 6502 struct
 *              operand -- memory where location to store y_index is
 *      OUTPUTS: none
 *      SIDE EFFECTS: none
 */
static void STY_operation(sf_t *sf, uint8_t *operand) {
    *operand = sf->y_index;
}

/* STA_operation
 *      DESCRIPTION: stores accumulator in memory
 *      INPUTS: sf -- 6502 struct
 *              operand -- memory where location to store accumulator is
 *      OUTPUTS: none
 *      SIDE EFFECTS: none
 */
static void STA_operation(sf_t *sf, uint8_t *operand) {
    *operand = sf->accumulator;
}

/* STX_operation
 *      DESCRIPTION: stores x_index in memory
 *      INPUTS: sf -- 6502 struct
 *              operand -- memory where location to store x_index is
 *      OUTPUTS: none
 *      SIDE EFFECTS: none
 */
static void STX_operation(sf_t *sf, uint8_t *operand) {
    *operand = sf->x_index;
}

/* LDY_operation
 *      DESCRIPTION: loads y_index with memory
 *      INPUTS: sf -- 6502 struct
 *              operand -- memory where value to store in y_index is
 *      OUTPUTS: none
 *      SIDE EFFECTS: modifies negative, zero flags according to y_index
 */
static void LDY_operation(sf_t *sf, uint8_t *operand) {
    sf->y_index = *operand;
    check_negative_and_zero(sf, sf->y_index);
}

/* LDA_operation
 *      DESCRIPTION: loads accumulator with memory
 *      INPUTS: sf -- 6502 struct
 *              operand -- memory where value to store in accumulator is
 *      OUTPUTS: none
 *      SIDE EFFECTS: none
 */
static void LDA_operation(sf_t *sf, uint8_t *operand) {
    sf->accumulator = *operand;
    check_negative_and_zero(sf, sf->accumulator);
}

/* LDX_operation
 *      DESCRIPTION: loads x_index with memory
 *      INPUTS: sf -- 6502 struct
 *              operand -- memory where value to store in x_index is
 *      OUTPUTS: none
 *      SIDE EFFECTS: none
 */
static void LDX_operation(sf_t *sf, uint8_t *operand) {
    sf->x_index = *operand;
    check_negative_and_zero(sf, sf->x_index);
}

/* open_bytecode
 *      DESCRIPTION: compare memory and y_index
 *      INPUTS: sf -- 6502 struct
 *              operand -- memory to compare to y_index
 *      OUTPUTS: none
 *      SIDE EFFECTS: sets carry, zero, and negative flags according to comparison result
 */
static void CPY_operation(sf_t *sf, uint8_t *operand) {
    if (sf->y_index >= *operand) {
        sf->status |= (1 << CARRY_INDEX);
    } else {
        sf->status &= ~(1 << CARRY_INDEX);
    }
    uint8_t temp = sf->y_index - *operand;

    check_negative_and_zero(sf, temp);
}

/* CMP_operation
 *      DESCRIPTION: compare memory and accumulator
 *      INPUTS: sf -- 6502 struct
 *              operand -- memory to compare to accumulator
 *      OUTPUTS: none
 *      SIDE EFFECTS: none
 */
static void CMP_operation(sf_t *sf, uint8_t *operand) {
    if (sf->accumulator >= *operand) {
        sf->status |= (1 << CARRY_INDEX);
    } else {
        sf->status &= ~(1 << CARRY_INDEX);
    }
    uint8_t temp = sf->accumulator - *operand;

    check_negative_and_zero(sf, temp);
}

/* DEC_operation
 *      DESCRIPTION: decrements memory
 *      INPUTS: sf -- 6502 struct
 *              operand -- memory with value to decrement
 *      OUTPUTS: none
 *      SIDE EFFECTS: modifies negative and zero flags
 */
static void DEC_operation(sf_t *sf, uint8_t *operand) {
    *operand -= 1;
    check_negative_and_zero(sf, *operand);
}

/* CPX_operation
 *      DESCRIPTION: compare memory and x_index
 *      INPUTS: sf -- 6502 struct
 *              operand -- memory to compare to x_index
 *      OUTPUTS: none
 *      SIDE EFFECTS: modifies carry, negative, and zero flags
 */
static void CPX_operation(sf_t *sf, uint8_t *operand) {
    if (sf->x_index >= *operand) {
        sf->status |= (1 << CARRY_INDEX);
    } else {
        sf->status &= ~(1 << CARRY_INDEX);
    }
    uint8_t temp = sf->x_index - *operand;

    check_negative_and_zero(sf, temp);
}

/* SBC_operation
 *      DESCRIPTION: subtracts memory from accumulator (with carry) and stores the result in accumulator, expects carry to be set for normal operation
 *      INPUTS: sf -- 6502 stryuct
 *              operand -- memory with value to subtract from accumulator
 *      OUTPUTS: none
 *      SIDE EFFECTS: modifies accumulator; carry, overflow, negative, and zero flags
 */
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

/* INC_operation
 *      DESCRIPTION: increments memory
 *      INPUTS: sf -- 6502 struct
 *              operand -- memory with value to increment
 *      OUTPUTS: none
 *      SIDE EFFECTS: modifies negative, zero flags
 */
static void INC_operation(sf_t *sf, uint8_t *operand) {
    *operand += 1;
    check_negative_and_zero(sf, *operand);
}

/* process_line
 *      DESCRIPTION: runs line of bytecode beginning at location of PC
 *      INPUTS: sf -- 6502 struct
 *      OUTPUTS: none
 *      SIDE EFFECTS: may modify status, accumulator, x_index, y_index, and pc depending on instruction
 */
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
            sf->pc += 2;
            if (!(sf->status & (1 << NEGATIVE_INDEX))) {
                sf->pc += (int8_t) sf->memory[sf->pc - 1]; // offset is stored in previous byte
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
            sf->pc += 2;
            if (sf->status & (1 << NEGATIVE_INDEX)) {
                sf->pc += (int8_t)sf->memory[sf->pc - 1];
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
            sf->pc += 2;
            if (!(sf->status & (1 << OVERFLOW_INDEX))) {
                sf->pc += (int8_t)sf->memory[sf->pc - 1];
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
            sf->pc += 2;
            if (sf->status & (1 << OVERFLOW_INDEX)) {
                sf->pc += (int8_t)sf->memory[sf->pc - 1];
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
            sf->pc += 2;
            if (!(sf->status & (1 << CARRY_INDEX))) {
                sf->pc += (int8_t)sf->memory[sf->pc - 1];
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
            sf->pc += 2;
            if (sf->status & (1 << CARRY_INDEX)) {
                sf->pc += (int8_t)sf->memory[sf->pc - 1];
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
            sf->pc += 2;
            if (!(sf->status & (1 << ZERO_INDEX))) {
                sf->pc += (int8_t)sf->memory[sf->pc - 1];
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
            sf->pc += 2;
            if (sf->status & (1 << ZERO_INDEX)) {
                sf->pc += (int8_t)sf->memory[sf->pc - 1];
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
