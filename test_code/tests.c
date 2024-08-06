#include <stdio.h>
#include <assert.h>

#include "tests.h"
#include "../lib/lib.h"
#include "../assembler/table.h"

/* OPCODE TESTS */

static int check_flags(uint8_t status, uint8_t negative, uint8_t overflow, uint8_t brk,
                        uint8_t decimal_mode, uint8_t interrupt_disable, uint8_t zero, uint8_t carry) {
    
    if (negative < 2) {
        if (((status & (1 << NEGATIVE_INDEX)) >> NEGATIVE_INDEX) != negative) {
            return -1;
        }
    }
    
    if (overflow < 2) {
        if (((status & (1 << OVERFLOW_INDEX)) >> OVERFLOW_INDEX) != overflow) {
            return -1;
        }
    }
    
    if (brk < 2) {
        if (((status & (1 << BREAK_INDEX)) >> BREAK_INDEX) != brk) {
            return -1;
        }
    }
    
    if (decimal_mode < 2) {
        if (((status & (1 << DECIMAL_INDEX)) >> DECIMAL_INDEX) != decimal_mode) {
            return -1;
        }
    }
    
    if (interrupt_disable < 2) {
        if (((status & (1 << INTERRUPT_INDEX)) >> INTERRUPT_INDEX) != interrupt_disable) {
            return -1;
        }
    }
    
    if (zero < 2) {
        if (((status & (1 << ZERO_INDEX)) >> ZERO_INDEX) != zero) {
            return -1;
        }
    }
    
    if (carry < 2) {
        if (((status & (1 << CARRY_INDEX)) >> CARRY_INDEX) != carry) {
            return -1;
        }
    }
    
    return 0;
}

// helper function for tests that should branch when flag is not set
static int branch_not_set_test(sf_t *sf, uint8_t opcode, uint8_t flag_index) {
    unsigned char offset = 0x76;
    sf->memory[ROM_START] = opcode;
    sf->memory[ROM_START + 1] = offset;
    sf->status |= (1 << flag_index);
    // test when flag = 1
    process_line(sf);
    if (sf->pc != ROM_START + 2) {
        return -1;
    }
    sf->pc = ROM_START;
    sf->status &= (~(1 << flag_index));
    // test when flag = 0
    process_line(sf);
    if (sf->pc != ROM_START + offset + 2) {
        return -1;
    }
    return 0;
}

// helper function for tests that should branch when flag is set
static int branch_set_test(sf_t *sf, uint8_t opcode, uint8_t flag_index) {
    int8_t offset = 0xF6;
    sf->memory[ROM_START] = opcode;
    sf->memory[ROM_START + 1] = offset;
    // test when flag = 0
    process_line(sf);
    if (sf->pc != ROM_START + 2) {
        return -1;
    }
    sf->pc = ROM_START;
    sf->status |= (1 << flag_index);
    // test when flag = 1
    process_line(sf);
    if (sf->pc != ROM_START + offset + 2) {
        return -1;
    }
    return 0;
}

static int BRK_RTI_TEST(sf_t *sf) {
    sf->memory[IRQ_ADDRESS] = OP_RTI;
    sf->memory[ROM_START] = OP_BRK;
    process_line(sf);
    if ((sf->status != ((1 << INTERRUPT_INDEX)|(1 << BREAK_INDEX))) || sf->pc != IRQ_ADDRESS) {
        return -1;
    }
    process_line(sf);
    if (sf->pc != (ROM_START + 1) || sf->status != 0) {
        return -1;
    }
    return 0;
}

static int PHP_PLP_TEST(sf_t *sf) {
    sf->memory[ROM_START] = OP_PHP;
    sf->memory[ROM_START + 1] = OP_PLP;
    sf->status = TEST_MAGIC;
    process_line(sf);
    sf->status = 0;
    process_line(sf);
    if (sf->status != TEST_MAGIC || sf->esp != STACK_START) {
        return -1;
    } 

    return 0;
}

static int BPL_TEST(sf_t *sf) {
    return branch_not_set_test(sf, OP_BPL, NEGATIVE_INDEX);
}

static int CLC_CLD_CLI_CLV_TEST(sf_t *sf) {
    sf->memory[ROM_START] = OP_CLC;
    sf->memory[ROM_START + 1] = OP_CLD;
    sf->memory[ROM_START + 2] = OP_CLI;
    sf->memory[ROM_START + 3] = OP_CLV;
    sf->status |= ((1 << CARRY_INDEX)|(1 << DECIMAL_INDEX)|(1 << INTERRUPT_INDEX)|(1 << OVERFLOW_INDEX));
    process_line(sf);
    process_line(sf);
    process_line(sf);
    process_line(sf);
    if (sf->status & ((1 << CARRY_INDEX)|(1 << DECIMAL_INDEX)|(1 << INTERRUPT_INDEX)|(1 << OVERFLOW_INDEX))) {
        return -1;
    }
    return 0;
}

static int JSR_RTS_TEST(sf_t *sf) {
    int sub_address = 0xB0B0;
    sf->memory[ROM_START] = OP_JSR;
    sf->memory[ROM_START + 1] = sub_address & 0x00FF;
    sf->memory[ROM_START + 2] = sub_address >> 8;
    sf->memory[sub_address] = OP_RTS;
    process_line(sf);
    if (sf->pc != sub_address) {
        return -1;
    }
    process_line(sf);
    if (sf->pc != ROM_START + 3) {
        return -1;
    }
    return 0;
}

static int BMI_TEST(sf_t *sf) {
    branch_set_test(sf, OP_BMI, NEGATIVE_INDEX);
}

static int SEC_SED_SEI_TEST(sf_t *sf) {
    sf->memory[ROM_START] = OP_SEC;
    sf->memory[ROM_START + 1] = OP_SED;
    sf->memory[ROM_START + 2] = OP_SEI;
    process_line(sf);
    process_line(sf);
    process_line(sf);
    if (sf->status != ((1 << CARRY_INDEX)|(1 << DECIMAL_INDEX)|(1 << INTERRUPT_INDEX))) {
        return -1;
    }
    return 0;
}

static int PHA_PLA_TEST(sf_t *sf) {
    sf->memory[ROM_START] = OP_PHA;
    sf->memory[ROM_START + 1] = OP_PLA;
    sf->accumulator = TEST_MAGIC;
    process_line(sf);
    sf->accumulator = 0;
    process_line(sf);
    if (sf->accumulator != TEST_MAGIC || sf->esp != STACK_START) {
        return -1;
    }
    return 0;
}

static int JMP_TEST(sf_t *sf) {
    int jump_addr = 0xB090;
    sf->memory[ROM_START] = OP_JMP;
    sf->memory[ROM_START + 1] = jump_addr & 0x00FF;
    sf->memory[ROM_START + 2] = jump_addr >> 8;
    process_line(sf);
    if (sf->pc != jump_addr) {
        return -1;
    }
    return 0;
    
}

static int BVC_TEST(sf_t *sf) {
    return branch_not_set_test(sf, OP_BVC, OVERFLOW_INDEX);
}

static int BVS_TEST(sf_t *sf) {
    return branch_set_test(sf, OP_BVS, OVERFLOW_INDEX);
}

static int JI_TEST(sf_t *sf) {
    int jump_addr = 0xB090;
    int ind_location = 0x215F;
    sf->memory[ROM_START] = OP_JI;
    sf->memory[ROM_START + 1] = ind_location & 0x00FF; 
    sf->memory[ROM_START + 2] = ind_location >> 8;
    sf->memory[ind_location] = jump_addr & 0x00FF; 
    sf->memory[ind_location + 1] = jump_addr >> 8;
    process_line(sf);
    if (sf->pc != jump_addr) {
        return -1;
    } 
    return 0;
}

static int TXA_TYA_TEST(sf_t *sf) {
    sf->memory[ROM_START] = OP_TXA;
    sf->memory[ROM_START + 1] = OP_TYA;
    sf->x_index = 0x81;
    sf->y_index = 0x34;
    process_line(sf);
    if ((sf->accumulator != sf->x_index) || check_flags(sf->status, 1, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }
    process_line(sf);
    if ((sf->accumulator != sf->y_index) || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }
    return 0;
}

static int BCC_TEST(sf_t *sf) {
    branch_not_set_test(sf, OP_BCC, CARRY_INDEX);
}

static int TXS_TEST(sf_t *sf) {
    sf->memory[ROM_START] = OP_TXS;
    sf->x_index = 0x12;
    process_line(sf);
    if (sf->esp != sf->x_index) {
        return -1;
    }
    return 0;
}

static int TAX_TAY_TEST(sf_t *sf) {
    sf->memory[ROM_START] = OP_TAX;
    sf->memory[ROM_START + 1] = OP_TAY;
    sf->accumulator = 0x76;
    process_line(sf);
    if ((sf->x_index != sf->accumulator) || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }
    process_line(sf);
    if ((sf->y_index != sf->accumulator) || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }
    return 0;
}

static int BCS_TEST(sf_t *sf) {
    branch_set_test(sf, OP_BCS, CARRY_INDEX);
}

static int TSX_TEST(sf_t *sf) {
    sf->memory[ROM_START] = OP_TSX;
    sf->esp = 0x00;
    process_line(sf);
    if ((sf->x_index != sf->esp) || check_flags(sf->status, 0, 2, 2, 2, 2, 1, 2)) {
        return -1;
    }
    return 0;
}

static int INX_INY_TEST(sf_t *sf) {
    sf->x_index = 0xFF;
    sf->y_index = 0x7F;
    sf->memory[ROM_START] = OP_INX;
    sf->memory[ROM_START + 1] = OP_INY;
    process_line(sf);
    if ((sf->x_index != 0x00) || check_flags(sf->status, 0, 2, 2, 2, 2, 1, 2)) {
        return -1;
    }
    process_line(sf);
    if ((sf->y_index != 0x80) || check_flags(sf->status, 1, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }
    return 0;
}

static int DEX_DEY_TEST(sf_t *sf) {
    sf->y_index = 0x01;
    sf->memory[ROM_START] = OP_DEX;
    sf->memory[ROM_START + 1] = OP_DEY;
    process_line(sf);
    if (sf->x_index != 0xFF || check_flags(sf->status, 1, 2, 2, 2, 2, 0, 2)) {
        return -1;
    } 
    process_line(sf);
    if ((sf->y_index != 0x00) || check_flags(sf->status, 0, 2, 2, 2, 2, 1, 2)) { // unsigned values will overflow
        return -1;
    }
    return 0;
}

static int BNE_TEST(sf_t *sf) {
    branch_not_set_test(sf, OP_BNE, ZERO_INDEX);
}

static int NOP_TEST(sf_t *sf) {
    sf->memory[ROM_START] = OP_NOP;
    process_line(sf);
    if (sf->pc != ROM_START + 1) {
        return -1;
    }
    return 0;
}

static int BEQ_TEST(sf_t *sf) {
    branch_set_test(sf, OP_BEQ, ZERO_INDEX);
}

// helper function to load memory with values for indirect x addressing mode
static void ind_x_load(sf_t *sf, uint8_t opcode, uint8_t accum_init, uint8_t or_value) {
    sf->accumulator = accum_init;
    sf->memory[ROM_START] = opcode | (ADDR_MODE_IND_X << 2);
    sf->memory[ROM_START + 1] = 0xFE; // test wraparound
    sf->x_index = 0x05;
    sf->memory[(0xFE + sf->x_index) % 0xFF] = 0x15;
    sf->memory[(0xFE + sf->x_index + 1) % 0xFF] = 0x24;
    sf->memory[0x2415] = or_value;
}

// helper function to load memory with values for zero-page addressing mode
static void zpg_load(sf_t *sf, uint8_t opcode, uint8_t accum_init, uint8_t or_value) {
    sf->accumulator = accum_init;
    sf->memory[ROM_START] = opcode | (ADDR_MODE_ZPG << 2);
    sf->memory[ROM_START + 1] = 0x3E;
    sf->memory[0x003E] = or_value;
}

// helper function to load memory with values for immediate addressing mode
static void imm_load(sf_t *sf, uint8_t opcode, uint8_t accum_init, uint8_t or_value) {
    sf->accumulator = accum_init;
    sf->memory[ROM_START] = opcode | (ADDR_MODE_IMM << 2);
    sf->memory[ROM_START + 1] = or_value;
}

// helper function to load memory with values for absolute addressing mode
static void abs_load(sf_t *sf, uint8_t opcode, uint8_t accum_init, uint8_t or_value) {
    sf->accumulator = accum_init;
    sf->memory[ROM_START] = opcode | (ADDR_MODE_ABS << 2);
    sf->memory[ROM_START + 1] = 0x3E;
    sf->memory[ROM_START + 2] = 0x20;
    sf->memory[0x203E] = or_value;
}

// helper function to load memory with values for indirect y addressing mode
static void ind_y_load(sf_t *sf, uint8_t opcode, uint8_t accum_init, uint8_t or_value) {
    sf->accumulator = accum_init;
    sf->memory[ROM_START] = opcode | (ADDR_MODE_IND_Y << 2);
    sf->memory[ROM_START + 1] = 0x4C;
    sf->y_index = 0x05;
    sf->memory[0x4C] = 0x00;
    sf->memory[0x4D] = 0x21;
    sf->memory[0x2100 + sf->y_index] = or_value;
}

// helper function to load memory with values for zero-page x-indexed addressing mode
static void zpg_x_load(sf_t *sf, uint8_t opcode, uint8_t accum_init, uint8_t or_value) {
    sf->accumulator = accum_init;
    sf->memory[ROM_START] = opcode | (ADDR_MODE_ZPG_X << 2);
    sf->memory[ROM_START + 1] = 0x3E;
    sf->y_index = 0x05;
    sf->memory[0x3E + sf->y_index] = or_value;
}

// helper function to load memory with values for absolute y-indexed addressing mode
static void abs_y_load(sf_t *sf, uint8_t opcode, uint8_t accum_init, uint8_t or_value) {
    sf->accumulator = accum_init;
    sf->memory[ROM_START] = opcode | (ADDR_MODE_ABS_Y << 2);
    sf->memory[ROM_START + 1] = 0x3E;
    sf->memory[ROM_START + 2] = 0x20;
    sf->y_index = 0x05;
    sf->memory[0x203E + sf->y_index] = or_value;
}

// helper function to load memory with values for absolute x-indexed addressing mode
static void abs_x_load(sf_t *sf, uint8_t opcode, uint8_t accum_init, uint8_t or_value) {
    sf->accumulator = accum_init;
    sf->memory[ROM_START] = opcode | (ADDR_MODE_ABS_X << 2);
    sf->memory[ROM_START + 1] = 0x3E;
    sf->memory[ROM_START + 2] = 0x20;
    sf->x_index = 0x05;
    sf->memory[0x203E + sf->x_index] = or_value;
}

static int ORA_TEST(sf_t *sf) {
    sf->x_index = 0x3D;
    sf->accumulator = 0x22;
    sf->memory[ROM_START] = OP_ORA | (ADDR_MODE_IND_X << 2);
    sf->memory[ROM_START + 1] = 0x91;
    sf->memory[0xCE] = 0xEF;
    sf->memory[0xCF] = 0xBE;
    sf->memory[0xBEEF] = 0x6E;
    process_line(sf);
    if (sf->accumulator != (0x22 | 0x6E) || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->accumulator = 0x92;
    sf->memory[ROM_START] = OP_ORA | (ADDR_MODE_ZPG << 2);
    sf->memory[ROM_START + 1] = 0x01;
    sf->memory[0x01] = 0x14;
    process_line(sf);
    if (sf->accumulator != (0x92 | 0x14) || check_flags(sf->status, 1, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->accumulator = 0x12;
    sf->memory[ROM_START] = OP_ORA | (ADDR_MODE_IMM << 2);
    sf->memory[ROM_START + 1] = 0x41;
    process_line(sf);
    if (sf->accumulator != (0x12 | 0x41) || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->accumulator = 0x00;
    sf->memory[ROM_START] = OP_ORA | (ADDR_MODE_ABS << 2);
    sf->memory[ROM_START + 1] = 0x2B;
    sf->memory[ROM_START + 2] = 0xEE;
    sf->memory[0xEE2B] = 0x00;
    process_line(sf);
    if (sf->accumulator != 0x00 || check_flags(sf->status, 0, 2, 2, 2, 2, 1, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->y_index = 0x08;
    sf->accumulator = 0x12;
    sf->memory[ROM_START] = OP_ORA | (ADDR_MODE_IND_Y << 2);
    sf->memory[ROM_START + 1] = 0xFF;
    sf->memory[0xFF] = 0x25;
    sf->memory[0x100] = 0x1D;
    sf->memory[0x1D2D] = 0x80;
    process_line(sf);
    if (sf->accumulator != 0x92 || check_flags(sf->status, 1, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->x_index = 0xF4;
    sf->accumulator = 0x00;
    sf->memory[ROM_START] = OP_ORA | (ADDR_MODE_ZPG_X << 2);
    sf->memory[ROM_START + 1] = 0x02;
    sf->memory[0xF6] = 0x67;
    process_line(sf);
    if (sf->accumulator != 0x67 || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->y_index = 0xE4;
    sf->accumulator = 0x72;
    sf->memory[ROM_START] = OP_ORA | (ADDR_MODE_ABS_Y << 2);
    sf->memory[ROM_START + 1] = 0x3A;
    sf->memory[ROM_START + 2] = 0xFF;
    sf->memory[(0xFF3A + 0xE4) % 0xFFFF] = 0x00;
    process_line(sf);
    if (sf->accumulator != 0x72 || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->x_index = 0xE4;
    sf->accumulator = 0x72;
    sf->memory[ROM_START] = OP_ORA | (ADDR_MODE_ABS_X << 2);
    sf->memory[ROM_START + 1] = 0x3A;
    sf->memory[ROM_START + 2] = 0xFF;
    sf->memory[(0xFF3A + 0xE4) % 0xFFFF] = 0x00;
    process_line(sf);
    if (sf->accumulator != 0x72 || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    return 0;
}

static int ASL_TEST(sf_t *sf) {
    sf->accumulator = 0x81;
    sf->memory[ROM_START] = OP_ASL | (ADDR_MODE_ACCUM << 2);
    process_line(sf);
    if (sf->accumulator != 0x02 || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 1)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->memory[ROM_START] = OP_ASL | (ADDR_MODE_ZPG << 2);
    sf->memory[ROM_START + 1] = 0x18;
    sf->memory[0x18] = 0x67;
    process_line(sf);
    if (sf->memory[0x18] != 0xCE || check_flags(sf->status, 1, 2, 2, 2, 2, 0, 0)) {
        return -1;
    } 

    sf->pc = ROM_START;
    sf->memory[ROM_START] = OP_ASL | (ADDR_MODE_ABS << 2);
    sf->memory[ROM_START + 1] = 0x15;
    sf->memory[ROM_START + 2] = 0x24;
    sf->memory[0x2415] = 0x80;
    process_line(sf);
    if (sf->memory[0x2415] != 0x00 || check_flags(sf->status, 0, 2, 2, 2, 2, 1, 1)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->x_index = 0x76;
    sf->memory[ROM_START] = OP_ASL | (ADDR_MODE_ZPG_X << 2);
    sf->memory[ROM_START + 1] = 0xFE;
    sf->memory[(0xFE + 0x76) % 0xFF] = 0x47;
    process_line(sf);
    if (sf->memory[(0xFE + 0x76) % 0xFF] != 0x8E || check_flags(sf->status, 1, 2, 2, 2, 2, 0, 0)) {
        return -1;
    } 

    sf->pc = ROM_START;
    sf->x_index = 0x17;
    sf->memory[ROM_START] = OP_ASL | (ADDR_MODE_ABS_X << 2);
    sf->memory[ROM_START + 1] = 0x15;
    sf->memory[ROM_START + 2] = 0x24;
    sf->memory[0x242C] = 0x47;
    process_line(sf);
    if (sf->memory[0x242C] != 0x8E || check_flags(sf->status, 1, 2, 2, 2, 2, 0, 0)) {
        return -1;
    }

    return 0;
}

static int BIT_TEST(sf_t *sf) {
    sf->memory[ROM_START] = OP_BIT | (ADDR_MODE_ZPG << 2);
    sf->memory[ROM_START + 1] = 0x21;
    sf->memory[0x21] = 0b00111111;
    sf->accumulator = 0b11000000;
    process_line(sf);
    if (check_flags(sf->status, 0, 0, 2, 2, 2, 1, 2)) {
        return -1;
    }
    
    sf->pc = ROM_START;
    sf->memory[ROM_START] = OP_BIT | (ADDR_MODE_ABS << 2);
    sf->memory[ROM_START + 2] = 0x12;
    sf->memory[0x1221] = 0b11000000;
    process_line(sf);
    if (check_flags(sf->status, 1, 1, 2, 2, 2, 0, 2)) {
        return -1;
    }

    return 0;
}

static int AND_TEST(sf_t *sf) {
    sf->x_index = 0x3D;
    sf->accumulator = 0x22;
    sf->memory[ROM_START] = OP_AND | (ADDR_MODE_IND_X << 2);
    sf->memory[ROM_START + 1] = 0x91;
    sf->memory[0xCE] = 0xEF;
    sf->memory[0xCF] = 0xBE;
    sf->memory[0xBEEF] = 0x6E;
    process_line(sf);
    if (sf->accumulator != (0x22 & 0x6E) || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->accumulator = 0x92;
    sf->memory[ROM_START] = OP_AND | (ADDR_MODE_ZPG << 2);
    sf->memory[ROM_START + 1] = 0x01;
    sf->memory[0x01] = 0x81;
    process_line(sf);
    if (sf->accumulator != (0x92 & 0x81) || check_flags(sf->status, 1, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->accumulator = 0x12;
    sf->memory[ROM_START] = OP_AND | (ADDR_MODE_IMM << 2);
    sf->memory[ROM_START + 1] = 0x41;
    process_line(sf);
    if (sf->accumulator != 0x00 || check_flags(sf->status, 0, 2, 2, 2, 2, 1, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->accumulator = 0x78;
    sf->memory[ROM_START] = OP_AND | (ADDR_MODE_ABS << 2);
    sf->memory[ROM_START + 1] = 0x2B;
    sf->memory[ROM_START + 2] = 0xEE;
    sf->memory[0xEE2B] = 0x48;
    process_line(sf);
    if (sf->accumulator != 0x48 || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->y_index = 0x08;
    sf->accumulator = 0xF3;
    sf->memory[ROM_START] = OP_AND | (ADDR_MODE_IND_Y << 2);
    sf->memory[ROM_START + 1] = 0xFF;
    sf->memory[0xFF] = 0x25;
    sf->memory[0x100] = 0x1D;
    sf->memory[0x1D2D] = 0x73;
    process_line(sf);
    if (sf->accumulator != 0x73 || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->x_index = 0xF4;
    sf->accumulator = 0x00;
    sf->memory[ROM_START] = OP_AND | (ADDR_MODE_ZPG_X << 2);
    sf->memory[ROM_START + 1] = 0x02;
    sf->memory[0xF6] = 0x67;
    process_line(sf);
    if (sf->accumulator != 0x00 || check_flags(sf->status, 0, 2, 2, 2, 2, 1, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->y_index = 0xE4;
    sf->accumulator = 0xF2;
    sf->memory[ROM_START] = OP_AND | (ADDR_MODE_ABS_Y << 2);
    sf->memory[ROM_START + 1] = 0x3A;
    sf->memory[ROM_START + 2] = 0xFF;
    sf->memory[(0xFF3A + 0xE4) % 0xFFFF] = 0x8F;
    process_line(sf);
    if (sf->accumulator != 0x82 || check_flags(sf->status, 1, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->x_index = 0xE4;
    sf->accumulator = 0xF2;
    sf->memory[ROM_START] = OP_AND | (ADDR_MODE_ABS_X << 2);
    sf->memory[ROM_START + 1] = 0x3A;
    sf->memory[ROM_START + 2] = 0xFF;
    sf->memory[(0xFF3A + 0xE4) % 0xFFFF] = 0x8F;
    process_line(sf);
    if (sf->accumulator != 0x82 || check_flags(sf->status, 1, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    return 0;
}

static int ROL_TEST(sf_t *sf) {
    sf->status |= (1 << CARRY_INDEX);
    sf->memory[ROM_START] = OP_ROL | (ADDR_MODE_ZPG << 2);
    sf->memory[ROM_START + 1] = 0x48;
    sf->memory[0x48] = 0xA1;
    process_line(sf);
    if (sf->memory[0x48] != (((0xA1 << 1) & 0xFF) + 1) || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 1)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->status = 0x00;
    sf->memory[ROM_START] = OP_ROL | (ADDR_MODE_ACCUM << 2);
    sf->accumulator = 0x48;
    process_line(sf);
    if (sf->accumulator != (0x48 << 1) || check_flags(sf->status, 1, 2, 2, 2, 2, 0, 0)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->status = (1 << CARRY_INDEX);
    sf->memory[ROM_START] = OP_ROL | (ADDR_MODE_ABS << 2);
    sf->memory[ROM_START + 1] = 0x54;
    sf->memory[ROM_START + 2] = 0x76;
    sf->memory[0x7654] = 0x63;
    process_line(sf);
    if (sf->memory[0x7654] != ((0x63 << 1) + 1) || check_flags(sf->status, 1, 2, 2, 2, 2, 0, 0)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->x_index = 0x07;
    sf->memory[ROM_START] = OP_ROL | (ADDR_MODE_ZPG_X << 2);
    sf->memory[ROM_START + 1] = 0x48;
    sf->memory[0x4F] = 0x80;
    process_line(sf);
    if (sf->memory[0x4F] != 0x00 || check_flags(sf->status, 0, 2, 2, 2, 2, 1, 1)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->status = 0x00;
    sf->x_index = 0x89;
    sf->memory[ROM_START] = OP_ROL | (ADDR_MODE_ABS_X << 2);
    sf->memory[ROM_START + 1] = 0x54;
    sf->memory[ROM_START + 2] = 0x76;
    sf->memory[0x7654 + 0x89] = 0x00;
    process_line(sf);
    if ((sf->memory[0x7654 + 0x89] != 0x00) || check_flags(sf->status, 0, 2, 2, 2, 2, 1, 0)) {
        return -1;
    }

    return 0;
}

static int EOR_TEST(sf_t *sf) {
    sf->x_index = 0x3D;
    sf->accumulator = 0x22;
    sf->memory[ROM_START] = OP_EOR | (ADDR_MODE_IND_X << 2);
    sf->memory[ROM_START + 1] = 0x91;
    sf->memory[0xCE] = 0xEF;
    sf->memory[0xCF] = 0xBE;
    sf->memory[0xBEEF] = 0x6E;
    process_line(sf);
    if (sf->accumulator != (0x22 ^ 0x6E) || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->accumulator = 0x72;
    sf->memory[ROM_START] = OP_EOR | (ADDR_MODE_ZPG << 2);
    sf->memory[ROM_START + 1] = 0x01;
    sf->memory[0x01] = 0x81;
    process_line(sf);
    if (sf->accumulator != 0xF3 || check_flags(sf->status, 1, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->accumulator = 0xA0;
    sf->memory[ROM_START] = OP_EOR | (ADDR_MODE_IMM << 2);
    sf->memory[ROM_START + 1] = 0xA0;
    process_line(sf);
    if (sf->accumulator != 0x00 || check_flags(sf->status, 0, 2, 2, 2, 2, 1, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->accumulator = 0x48;
    sf->memory[ROM_START] = OP_EOR | (ADDR_MODE_ABS << 2);
    sf->memory[ROM_START + 1] = 0x2B;
    sf->memory[ROM_START + 2] = 0xEE;
    sf->memory[0xEE2B] = 0x20;
    process_line(sf);
    if (sf->accumulator != 0x68 || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->y_index = 0x08;
    sf->accumulator = 0xF3;
    sf->memory[ROM_START] = OP_EOR | (ADDR_MODE_IND_Y << 2);
    sf->memory[ROM_START + 1] = 0xFF;
    sf->memory[0xFF] = 0x25;
    sf->memory[0x100] = 0x1D;
    sf->memory[0x1D2D] = 0x83;
    process_line(sf);
    if (sf->accumulator != 0x70 || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->x_index = 0xF4;
    sf->accumulator = 0x00;
    sf->memory[ROM_START] = OP_EOR | (ADDR_MODE_ZPG_X << 2);
    sf->memory[ROM_START + 1] = 0x02;
    sf->memory[0xF6] = 0x67;
    process_line(sf);
    if (sf->accumulator != 0x67 || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->y_index = 0xE4;
    sf->accumulator = 0xF2;
    sf->memory[ROM_START] = OP_EOR | (ADDR_MODE_ABS_Y << 2);
    sf->memory[ROM_START + 1] = 0x3A;
    sf->memory[ROM_START + 2] = 0xFF;
    sf->memory[(0xFF3A + 0xE4) % 0xFFFF] = 0x34;
    process_line(sf);
    if (sf->accumulator != 0xC6 || check_flags(sf->status, 1, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->x_index = 0xE4;
    sf->accumulator = 0xF2;
    sf->memory[ROM_START] = OP_EOR | (ADDR_MODE_ABS_X << 2);
    sf->memory[ROM_START + 1] = 0x3A;
    sf->memory[ROM_START + 2] = 0xFF;
    sf->memory[(0xFF3A + 0xE4) % 0xFFFF] = 0x34;
    process_line(sf);
    if (sf->accumulator != 0xC6 || check_flags(sf->status, 1, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    return 0;
}

static int LSR_TEST(sf_t *sf) {
    sf->memory[ROM_START] = OP_LSR | (ADDR_MODE_ZPG << 2);
    sf->memory[ROM_START + 1] = 0x48;
    sf->memory[0x48] = 0xA1;
    process_line(sf);
    if (sf->memory[0x48] != (0xA1 >> 1) || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 1)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->memory[ROM_START] = OP_LSR | (ADDR_MODE_ACCUM << 2);
    sf->accumulator = 0x48;
    process_line(sf);
    if (sf->accumulator != (0x48 >> 1) || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 0)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->memory[ROM_START] = OP_LSR | (ADDR_MODE_ABS << 2);
    sf->memory[ROM_START + 1] = 0x54;
    sf->memory[ROM_START + 2] = 0x76;
    sf->memory[0x7654] = 0x00;
    process_line(sf);
    if (sf->memory[0x7654] != 0x00 || check_flags(sf->status, 0, 2, 2, 2, 2, 1, 0)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->memory[ROM_START] = OP_LSR | (ADDR_MODE_ZPG_X << 2);
    sf->memory[ROM_START + 1] = 0x48;
    sf->x_index = 0x07;
    sf->memory[0x4F] = 0x01;
    process_line(sf);
    if (sf->memory[0x4F] != 0x00 || check_flags(sf->status, 0, 2, 2, 2, 2, 1, 1)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->status = 0x00;
    sf->memory[ROM_START] = OP_LSR | (ADDR_MODE_ABS_X << 2);
    sf->memory[ROM_START + 1] = 0x54;
    sf->memory[ROM_START + 2] = 0x76;
    sf->x_index = 0x89;
    sf->memory[0x7654 + 0x89] = 0x62;
    process_line(sf);
    if (sf->memory[0x7654 + 0x89] != (0x62 >> 1) || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 0)) {
        return -1;
    }

    return 0;
}

static int ADC_TEST(sf_t *sf) {
    sf->x_index = 0x3D;
    sf->accumulator = 0x22;
    sf->memory[ROM_START] = OP_ADC | (ADDR_MODE_IND_X << 2);
    sf->memory[ROM_START + 1] = 0x91;
    sf->memory[0xCE] = 0xEF;
    sf->memory[0xCF] = 0xBE;
    sf->memory[0xBEEF] = 0x6E;
    process_line(sf);
    if (sf->accumulator != 0x90 || check_flags(sf->status, 1, 1, 2, 2, 2, 0, 0)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->accumulator = 0xFE;
    sf->memory[ROM_START] = OP_ADC | (ADDR_MODE_ZPG << 2);
    sf->memory[ROM_START + 1] = 0x01;
    sf->memory[0x01] = 0x81;
    process_line(sf);
    if (sf->accumulator != 0x7F || check_flags(sf->status, 0, 1, 2, 2, 2, 0, 1)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->accumulator = 0x21;
    sf->memory[ROM_START] = OP_ADC | (ADDR_MODE_IMM << 2);
    sf->memory[ROM_START + 1] = 0x42;
    process_line(sf);
    if (sf->accumulator != 0x64 || check_flags(sf->status, 0, 0, 2, 2, 2, 0, 0)) {
        return -1;
    }
    
    sf->pc = ROM_START;
    sf->accumulator = 0xFF;
    sf->memory[ROM_START] = OP_ADC | (ADDR_MODE_ABS << 2);
    sf->memory[ROM_START + 1] = 0x2B;
    sf->memory[ROM_START + 2] = 0xEE;
    sf->memory[0xEE2B] = 0x01;
    process_line(sf);
    if (sf->accumulator != 0x00 || check_flags(sf->status, 0, 0, 2, 2, 2, 1, 1)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->y_index = 0x08;
    sf->status |= (1 << DECIMAL_INDEX);
    sf->accumulator = 0x49;
    sf->memory[ROM_START] = OP_ADC | (ADDR_MODE_IND_Y << 2);
    sf->memory[ROM_START + 1] = 0xFF;
    sf->memory[0xFF] = 0x25;
    sf->memory[0x100] = 0x1D;
    sf->memory[0x1D2D] = 0x40;
    process_line(sf);
    if (sf->accumulator != 0x90 || check_flags(sf->status, 1, 1, 2, 1, 2, 0, 0)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->x_index = 0xF4;
    sf->accumulator = 0x80;
    sf->memory[ROM_START] = OP_ADC | (ADDR_MODE_ZPG_X << 2);
    sf->memory[ROM_START + 1] = 0x02;
    sf->memory[0xF6] = 0x20;
    process_line(sf);
    if (sf->accumulator != 0x00 || check_flags(sf->status, 0, 0, 2, 1, 2, 1, 1)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->y_index = 0xE4;
    sf->accumulator = 0x99;
    sf->memory[ROM_START] = OP_ADC | (ADDR_MODE_ABS_Y << 2);
    sf->memory[ROM_START + 1] = 0x3A;
    sf->memory[ROM_START + 2] = 0x33;
    sf->memory[0x333A + 0xE4] = 0x99;
    process_line(sf);
    if (sf->accumulator != 0x99 || check_flags(sf->status, 1, 0, 2, 1, 2, 0, 1)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->x_index = 0xE4;
    sf->accumulator = 0x87;
    sf->memory[ROM_START] = OP_ADC | (ADDR_MODE_ABS_X << 2);
    sf->memory[ROM_START + 1] = 0x3A;
    sf->memory[ROM_START + 2] = 0x33;
    sf->memory[0x333A + 0xE4] = 0x83;
    process_line(sf);
    if (sf->accumulator != 0x71 || check_flags(sf->status, 0, 1, 2, 1, 2, 0, 1)) {
        return -1;
    }

    return 0;
}

static int ROR_TEST(sf_t *sf) {
    sf->memory[ROM_START] = OP_ROR | (ADDR_MODE_ZPG << 2);
    sf->memory[ROM_START + 1] = 0x48;
    sf->memory[0x48] = 0xA1;
    sf->status |= (1 << CARRY_INDEX);
    process_line(sf);
    if ((sf->memory[0x48] != ((0xA1 >> 1) | 0x80)) || check_flags(sf->status, 1, 2, 2, 2, 2, 0, 1)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->status = 0x00;
    sf->memory[ROM_START] = OP_ROR | (ADDR_MODE_ACCUM << 2);
    sf->accumulator = 0x48;
    process_line(sf);
    if ((sf->accumulator != (0x48 >> 1)) || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 0)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->status = 0x00;
    sf->memory[ROM_START] = OP_ROR | (ADDR_MODE_ABS << 2);
    sf->memory[ROM_START + 1] = 0x54;
    sf->memory[ROM_START + 2] = 0x76;
    sf->memory[0x7654] = 0x01;
    process_line(sf);
    if ((sf->memory[0x7654] != 0x00) || check_flags(sf->status, 0, 2, 2, 2, 2, 1, 1)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->status = 0x00;
    sf->memory[ROM_START] = OP_ROR | (ADDR_MODE_ZPG_X << 2);
    sf->memory[ROM_START + 1] = 0x48;
    sf->x_index = 0x07;
    sf->memory[0x4F] = 0x03;
    process_line(sf);
    if ((sf->memory[0x4F] != (0x03 >> 1)) || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 1)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->status = 0x00;
    sf->status |= (1 << CARRY_INDEX);
    sf->memory[ROM_START] = OP_ROR | (ADDR_MODE_ABS_X << 2);
    sf->memory[ROM_START + 1] = 0x54;
    sf->memory[ROM_START + 2] = 0x76;
    sf->x_index = 0x89;
    sf->memory[0x7654 + 0x89] = 0x34;
    process_line(sf);
    if ((sf->memory[0x7654 + 0x89] != ((0x34 >> 1) | 0x80)) || check_flags(sf->status, 1, 2, 2, 2, 2, 0, 0)) {
        return -1;
    }

    return 0;
}

static int STY_TEST(sf_t *sf) {
    sf->y_index = 0x71;
    sf->memory[ROM_START] = OP_STY | (ADDR_MODE_ZPG << 2);
    sf->memory[ROM_START + 1] = 0x48;
    process_line(sf);
    if (sf->memory[0x48] != sf->y_index) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->memory[ROM_START] = OP_STY | (ADDR_MODE_ABS << 2);
    sf->memory[ROM_START + 1] = 0x34;
    sf->memory[ROM_START + 2] = 0x12;
    process_line(sf);
    if (sf->memory[0x1234] != sf->y_index) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->x_index = 0x13;
    sf->memory[ROM_START] = OP_STY | (ADDR_MODE_ZPG_X << 2);
    sf->memory[ROM_START + 1] = 0x48;
    process_line(sf);
    if (sf->memory[0x48 + 0x13] != sf->y_index) {
        return -1;
    }

    return 0;
}

static int STA_TEST(sf_t *sf) {
    sf->accumulator = 0x71;
    sf->x_index = 0x05;
    sf->memory[ROM_START] = OP_STA | (ADDR_MODE_IND_X << 2);
    sf->memory[ROM_START + 1] = 0x38;
    sf->memory[0x38 + 0x05] = 0x53;
    sf->memory[0x38 + 0x05 + 0x01] = 0x54;
    process_line(sf);
    if (sf->memory[0x5453] != sf->accumulator) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->memory[ROM_START] = OP_STA | (ADDR_MODE_ZPG << 2);
    sf->memory[ROM_START + 1] = 0x38;
    process_line(sf);
    if (sf->memory[0x38] != sf->accumulator) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->memory[ROM_START] = OP_STA | (ADDR_MODE_ABS << 2);
    sf->memory[ROM_START + 1] = 0x34;
    sf->memory[ROM_START + 2] = 0x12;
    process_line(sf);
    if (sf->memory[0x1234] != sf->accumulator) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->y_index = 0x06;
    sf->memory[ROM_START] = OP_STA | (ADDR_MODE_IND_Y << 2);
    sf->memory[ROM_START + 1] = 0x49;
    sf->memory[0x49] = 0x53;
    sf->memory[0x4A] = 0x54;
    process_line(sf);
    if (sf->memory[0x5459] != sf->accumulator) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->x_index = 0x13;
    sf->memory[ROM_START] = OP_STA | (ADDR_MODE_ZPG_X << 2);
    sf->memory[ROM_START + 1] = 0x48;
    process_line(sf);
    if (sf->memory[0x48 + 0x13] != sf->accumulator) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->y_index = 0x15;
    sf->memory[ROM_START] = OP_STA | (ADDR_MODE_ABS_Y << 2);
    sf->memory[ROM_START + 1] = 0x34;
    sf->memory[ROM_START + 2] = 0x12;
    process_line(sf);
    if (sf->memory[0x1234 + 0x15] != sf->accumulator) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->x_index = 0x27;
    sf->memory[ROM_START] = OP_STA | (ADDR_MODE_ABS_X << 2);
    sf->memory[ROM_START + 1] = 0x78;
    sf->memory[ROM_START + 2] = 0x56;
    process_line(sf);
    if (sf->memory[0x5678 + 0x27] != sf->accumulator) {
        return -1;
    }

    return 0;
}

static int STX_TEST(sf_t *sf) {
    sf->x_index = 0x71;
    sf->memory[ROM_START] = OP_STX | (ADDR_MODE_ZPG << 2);
    sf->memory[ROM_START + 1] = 0x48;
    process_line(sf);
    if (sf->memory[0x48] != sf->x_index) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->memory[ROM_START] = OP_STX | (ADDR_MODE_ABS << 2);
    sf->memory[ROM_START + 1] = 0x34;
    sf->memory[ROM_START + 2] = 0x12;
    process_line(sf);
    if (sf->memory[0x1234] != sf->x_index) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->y_index = 0x13;
    sf->memory[ROM_START] = OP_STX | (ADDR_MODE_ZPG_Y << 2);
    sf->memory[ROM_START + 1] = 0x48;
    process_line(sf);
    if (sf->memory[0x48 + 0x13] != sf->x_index) {
        return -1;
    }

    return 0;
}

static int LDY_TEST(sf_t *sf) {
    sf->memory[ROM_START] = OP_LDY | (ADDR_MODE_ZPG << 2);
    sf->memory[ROM_START + 1] = 0x21;
    sf->memory[0x21] = 0x19;
    process_line(sf);
    if (sf->y_index != 0x19 || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->memory[ROM_START] = OP_LDY; // special case, immediate is 0x00
    sf->memory[ROM_START + 1] = 0x80;
    process_line(sf);
    if (sf->y_index != 0x80 || check_flags(sf->status, 1, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->memory[ROM_START] = OP_LDY | (ADDR_MODE_ABS << 2);
    sf->memory[ROM_START + 1] = 0x81;
    sf->memory[ROM_START + 2] = 0x72;
    sf->memory[0x7281] = 0x00;
    process_line(sf);
    if (sf->y_index != 0x00 || check_flags(sf->status, 0, 2, 2, 2, 2, 1, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->x_index = 0x12;
    sf->memory[ROM_START] = OP_LDY | (ADDR_MODE_ZPG_X << 2);
    sf->memory[ROM_START + 1] = 0xF8;
    sf->memory[(0x12 + 0xF8) % 0xFF] = 0x46;
    process_line(sf);
    if (sf->y_index != 0x46 || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->x_index = 0xC9;
    sf->memory[ROM_START] = OP_LDY | (ADDR_MODE_ABS_X << 2);
    sf->memory[ROM_START + 1] = 0x09;
    sf->memory[ROM_START + 2] = 0x12;
    sf->memory[0x1209 + 0xC9] = 0x08;
    process_line(sf);
    if (sf->y_index != 0x08 || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }
    
    return 0;
}

static int LDA_TEST(sf_t *sf) {
    sf->x_index = 0xC8;
    sf->memory[ROM_START] = OP_LDA | (ADDR_MODE_IND_X << 2);
    sf->memory[ROM_START + 1] = 0x57;
    sf->memory[(0x57 + 0xC8) % 0xFF] = 0x36;
    sf->memory[(0x57 + 0xC8 + 0x01) % 0xFF] = 0x63;
    sf->memory[0x6336] = 0x01;
    process_line(sf);
    if (sf->accumulator != 0x01 || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->memory[ROM_START] = OP_LDA | (ADDR_MODE_ZPG << 2);
    sf->memory[ROM_START + 1] = 0x21;
    sf->memory[0x21] = 0x19;
    process_line(sf);
    if (sf->accumulator != 0x19 || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->memory[ROM_START] = OP_LDA | (ADDR_MODE_IMM << 2);
    sf->memory[ROM_START + 1] = 0x80;
    process_line(sf);
    if (sf->accumulator != 0x80 || check_flags(sf->status, 1, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->memory[ROM_START] = OP_LDA | (ADDR_MODE_ABS << 2);
    sf->memory[ROM_START + 1] = 0x81;
    sf->memory[ROM_START + 2] = 0x72;
    sf->memory[0x7281] = 0x00;
    process_line(sf);
    if (sf->accumulator != 0x00 || check_flags(sf->status, 0, 2, 2, 2, 2, 1, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->y_index = 0x21;
    sf->memory[ROM_START] = OP_LDA | (ADDR_MODE_IND_Y << 2);
    sf->memory[ROM_START + 1] = 0x83;
    sf->memory[0x83] = 0x61;
    sf->memory[0x84] = 0x41;
    sf->memory[0x4161 + 0x21] = 0x98;
    process_line(sf);
    if (sf->accumulator != 0x98 || check_flags(sf->status, 1, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->x_index = 0x12;
    sf->memory[ROM_START] = OP_LDA | (ADDR_MODE_ZPG_X << 2);
    sf->memory[ROM_START + 1] = 0xF8;
    sf->memory[(0x12 + 0xF8) % 0xFF] = 0x46;
    process_line(sf);
    if (sf->accumulator != 0x46 || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->y_index = 0x31;
    sf->memory[ROM_START] = OP_LDA | (ADDR_MODE_ABS_Y << 2);
    sf->memory[ROM_START + 1] = 0x21;
    sf->memory[ROM_START + 2] = 0x34;
    sf->memory[0x3421 + 0x31] = 0x17;
    process_line(sf);
    if (sf->accumulator != 0x17 || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->x_index = 0xC9;
    sf->memory[ROM_START] = OP_LDA | (ADDR_MODE_ABS_X << 2);
    sf->memory[ROM_START + 1] = 0x09;
    sf->memory[ROM_START + 2] = 0x12;
    sf->memory[0x1209 + 0xC9] = 0x08;
    process_line(sf);
    if (sf->accumulator != 0x08 || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }
    
    return 0;
}

static int LDX_TEST(sf_t *sf) {
    sf->memory[ROM_START] = OP_LDX | (ADDR_MODE_ZPG << 2);
    sf->memory[ROM_START + 1] = 0x21;
    sf->memory[0x21] = 0x19;
    process_line(sf);
    if (sf->x_index != 0x19 || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->memory[ROM_START] = OP_LDX; // special case, immediate is 0x00
    sf->memory[ROM_START + 1] = 0x80;
    process_line(sf);
    if (sf->x_index != 0x80 || check_flags(sf->status, 1, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->memory[ROM_START] = OP_LDX | (ADDR_MODE_ABS << 2);
    sf->memory[ROM_START + 1] = 0x81;
    sf->memory[ROM_START + 2] = 0x72;
    sf->memory[0x7281] = 0x00;
    process_line(sf);
    if (sf->x_index != 0x00 || check_flags(sf->status, 0, 2, 2, 2, 2, 1, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->y_index = 0x12;
    sf->memory[ROM_START] = OP_LDX | (ADDR_MODE_ZPG_Y << 2);
    sf->memory[ROM_START + 1] = 0xF8;
    sf->memory[(0x12 + 0xF8) % 0xFF] = 0x46;
    process_line(sf);
    if (sf->x_index != 0x46 || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->y_index = 0xC9;
    sf->memory[ROM_START] = OP_LDX | (ADDR_MODE_ABS_X << 2);
    sf->memory[ROM_START + 1] = 0x09;
    sf->memory[ROM_START + 2] = 0x12;
    sf->memory[0x1209 + 0xC9] = 0x08;
    process_line(sf);
    if (sf->x_index != 0x08 || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }
    
    return 0;
}

static int CPY_TEST(sf_t *sf) {
    sf->y_index = 0x92;
    sf->memory[ROM_START] = OP_CPY;
    sf->memory[ROM_START + 1] = 0x92;
    process_line(sf);
    if (check_flags(sf->status, 0, 2, 2, 2, 2, 1, 1)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->y_index = 0x01;
    sf->memory[ROM_START] = OP_CPY | (ADDR_MODE_ZPG << 2);
    sf->memory[ROM_START + 1] = 0x41;
    sf->memory[0x41] = 0xFE;
    process_line(sf);
    if (check_flags(sf->status, 0, 2, 2, 2, 2, 0, 0)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->y_index = 0xC1;
    sf->memory[ROM_START] = OP_CPY | (ADDR_MODE_ABS << 2);
    sf->memory[ROM_START + 1] = 0x67;
    sf->memory[ROM_START + 2] = 0x13;
    sf->memory[0x1367] = 0x02;
    process_line(sf);
    if (check_flags(sf->status, 1, 2, 2, 2, 2, 0, 1)) {
        return -1;
    }

    return 0;
}

static int CMP_TEST(sf_t *sf) {
    sf->x_index = 0x41;
    sf->accumulator = 0x01;
    sf->memory[ROM_START] = OP_CMP | (ADDR_MODE_IND_X << 2);
    sf->memory[ROM_START + 1] = 0x42;
    sf->memory[0x83] = 0x49;
    sf->memory[0x84] = 0x25;
    sf->memory[0x2549] = 0x02;
    process_line(sf);
    if (check_flags(sf->status, 1, 2, 2, 2, 2, 0, 0)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->accumulator = 0xD1;
    sf->memory[ROM_START] = OP_CMP | (ADDR_MODE_ZPG << 2);
    sf->memory[ROM_START + 1] = 0xD2;
    sf->memory[0xD2] = 0x13;
    process_line(sf);
    if (check_flags(sf->status, 1, 2, 2, 2, 2, 0, 1)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->accumulator = 0xB6;
    sf->memory[ROM_START] = OP_CMP | (ADDR_MODE_IMM << 2);
    sf->memory[ROM_START + 1] = 0xB6;
    process_line(sf);
    if (check_flags(sf->status, 0, 2, 2, 2, 2, 1, 1)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->accumulator = 0x73;
    sf->memory[ROM_START] = OP_CMP | (ADDR_MODE_ABS << 2);
    sf->memory[ROM_START + 1] = 0xD9;
    sf->memory[ROM_START + 2] = 0x7D;
    sf->memory[0x7DD9] = 0x24;
    process_line(sf);
    if (check_flags(sf->status, 0, 2, 2, 2, 2, 0, 1)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->accumulator = 0x70;
    sf->y_index = 0x03;
    sf->memory[ROM_START] = OP_CMP | (ADDR_MODE_IND_Y << 2);
    sf->memory[ROM_START + 1] = 0xA9;
    sf->memory[0xA9] = 0x7B;
    sf->memory[0xAA] = 0x06;
    sf->memory[0x067E] = 0xFF;
    process_line(sf);
    if (check_flags(sf->status, 0, 2, 2, 2, 2, 0, 0)) {
        return -1;
    }
    
    sf->pc = ROM_START;
    sf->accumulator = 0x02;
    sf->x_index = 0x5C;
    sf->memory[ROM_START] = OP_CMP | (ADDR_MODE_ZPG_X << 2);
    sf->memory[ROM_START + 1] = 0x4B;
    sf->memory[0x5C + 0x4B] = 0x01;
    process_line(sf);
    if (check_flags(sf->status, 0, 2, 2, 2, 2, 0, 1)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->y_index = 0xF0;
    sf->memory[ROM_START] = OP_CMP | (ADDR_MODE_ABS_Y << 2);
    sf->memory[ROM_START + 1] = 0x01;
    sf->memory[ROM_START + 2] = 0x10;
    sf->memory[0x1001 + 0xF0] = 0x01;
    process_line(sf);
    if (check_flags(sf->status, 0, 2, 2, 2, 2, 0, 1)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->x_index = 0x12;
    sf->memory[ROM_START] = OP_CMP | (ADDR_MODE_ABS_X << 2);
    sf->memory[ROM_START + 1] = 0x02;
    sf->memory[ROM_START + 2] = 0x20;
    sf->memory[0x2002 + 0x12] = 0x01;
    process_line(sf);
    if (check_flags(sf->status, 0, 2, 2, 2, 2, 0, 1)) {
        return -1;
    }

    return 0;
}

static int DEC_TEST(sf_t *sf) {
    sf->memory[ROM_START] = OP_DEC | (ADDR_MODE_ZPG << 2);
    sf->memory[ROM_START + 1] = 0x89;
    sf->memory[0x89] = 0x01;
    process_line(sf);
    if (sf->memory[0x89] != 0x00 || check_flags(sf->status, 0, 2, 2, 2, 2, 1, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->memory[ROM_START] = OP_DEC | (ADDR_MODE_ABS << 2);
    sf->memory[ROM_START + 1] = 0x7B;
    sf->memory[ROM_START + 2] = 0x7B;
    sf->memory[0x7B7B] = 0x00;
    process_line(sf);
    if (sf->memory[0x7B7B] != 0xFF || check_flags(sf->status, 1, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->x_index = 0x90;
    sf->memory[ROM_START] = OP_DEC | (ADDR_MODE_ZPG_X << 2);
    sf->memory[ROM_START + 1] = 0xC6;
    sf->memory[(0xC6 + 0x90) % 0xFF] = 0xFF;
    process_line(sf);
    if (sf->memory[(0xC6 + 0x90) % 0xFF] != 0xFE || check_flags(sf->status, 1, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->x_index = 0xE4;
    sf->memory[ROM_START] = OP_DEC | (ADDR_MODE_ABS_X << 2);
    sf->memory[ROM_START + 1] = 0x08;
    sf->memory[ROM_START + 2] = 0xEE;
    sf->memory[0xEE08 + 0xE4] = 0x80;
    process_line(sf);
    if (sf->memory[0xEE08 + 0xE4] != 0x7F || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }
    
    return 0;
}

static int CPX_TEST(sf_t *sf) {
    sf->x_index = 0x92;
    sf->memory[ROM_START] = OP_CPX;
    sf->memory[ROM_START + 1] = 0x92;
    process_line(sf);
    if (check_flags(sf->status, 0, 2, 2, 2, 2, 1, 1)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->x_index = 0x01;
    sf->memory[ROM_START] = OP_CPX | (ADDR_MODE_ZPG << 2);
    sf->memory[ROM_START + 1] = 0x41;
    sf->memory[0x41] = 0xFE;
    process_line(sf);
    if (check_flags(sf->status, 0, 2, 2, 2, 2, 0, 0)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->x_index = 0xC1;
    sf->memory[ROM_START] = OP_CPX | (ADDR_MODE_ABS << 2);
    sf->memory[ROM_START + 1] = 0x67;
    sf->memory[ROM_START + 2] = 0x13;
    sf->memory[0x1367] = 0x02;
    process_line(sf);
    if (check_flags(sf->status, 1, 2, 2, 2, 2, 0, 1)) {
        return -1;
    }

    return 0;
}

static int SBC_TEST(sf_t *sf) {
    sf->x_index = 0x06;
    sf->status |= (1 << CARRY_INDEX); // for SBC, carry must be set for normal operation
    sf->accumulator = 0x7F;
    sf->memory[ROM_START] = OP_SBC | (ADDR_MODE_IND_X << 2);
    sf->memory[ROM_START + 1] = 0x71;
    sf->memory[0x77] = 0x91;
    sf->memory[0x78] = 0x09;
    sf->memory[0x0991] = 0x05;
    process_line(sf);
    if (sf->accumulator != 0x7A || check_flags(sf->status, 0, 0, 2, 2, 2, 0, 1)) {
        return -1;
    }
    
    sf->pc = ROM_START;
    sf->status &= ~(1 << CARRY_INDEX);
    sf->accumulator = 0x7F;
    sf->memory[ROM_START] = OP_SBC | (ADDR_MODE_ZPG << 2);
    sf->memory[ROM_START + 1] = 0xC6;
    sf->memory[0xC6] = 0x05;
    process_line(sf);
    if (sf->accumulator != 0x79 || check_flags(sf->status, 0, 0, 2, 2, 2, 0, 1)) {
        return -1;
    }
    
    sf->pc = ROM_START;
    sf->status |= (1 << CARRY_INDEX);
    sf->accumulator = 0x7F;
    sf->memory[ROM_START] = OP_SBC | (ADDR_MODE_IMM << 2);
    sf->memory[ROM_START + 1] = 0xFE;
    process_line(sf);
    if (sf->accumulator != 0x81 || check_flags(sf->status, 1, 1, 2, 2, 2, 0, 0)) {
        return -1;
    }
    
    sf->pc = ROM_START;
    sf->status |= (1 << CARRY_INDEX);
    sf->accumulator = 0x80;
    sf->memory[ROM_START] = OP_SBC | (ADDR_MODE_ABS << 2);
    sf->memory[ROM_START + 1] = 0x89;
    sf->memory[ROM_START + 2] = 0x67;
    sf->memory[0x6789] = 0x10;
    process_line(sf);
    if (sf->accumulator != 0x70 || check_flags(sf->status, 0, 1, 2, 2, 2, 0, 1)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->status |= (1 << DECIMAL_INDEX);
    sf->status |= (1 << CARRY_INDEX);
    sf->y_index = 0x54;
    sf->accumulator = 0x19;
    sf->memory[ROM_START] = OP_SBC | (ADDR_MODE_IND_Y << 2);
    sf->memory[ROM_START + 1] = 0x67;
    sf->memory[0x67] = 0x28;
    sf->memory[0x68] = 0x91;
    sf->memory[0x9128 + 0x54] = 0x19;
    process_line(sf);
    if (sf->accumulator != 0x00 || check_flags(sf->status, 0, 0, 2, 1, 2, 1, 1)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->status &= ~(1 << CARRY_INDEX);
    sf->x_index = 0x61;
    sf->accumulator = 0x23;
    sf->memory[ROM_START] = OP_SBC | (ADDR_MODE_ZPG_X << 2);
    sf->memory[ROM_START + 1] = 0x89;
    sf->memory[0x89 + 0x61] = 0x41;
    process_line(sf);
    if (sf->accumulator != 0x80 || check_flags(sf->status, 1, 0, 2, 1, 2, 0, 0)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->status |= (1 << CARRY_INDEX);
    sf->y_index = 0x02;
    sf->accumulator = 0x91;
    sf->memory[ROM_START] = OP_SBC | (ADDR_MODE_ABS_Y << 2);
    sf->memory[ROM_START + 1] = 0xB8;
    sf->memory[ROM_START + 2] = 0xAB;
    sf->memory[0xABBA] = 0x12;
    process_line(sf);
    if (sf->accumulator != 0x79 || check_flags(sf->status, 0, 1, 2, 1, 2, 0, 1)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->status |= (1 << CARRY_INDEX);
    sf->x_index = 0x07;
    sf->accumulator = 0x76;
    sf->memory[ROM_START] = OP_SBC | (ADDR_MODE_ABS_X << 2);
    sf->memory[ROM_START + 1] = 0xB7;
    sf->memory[ROM_START + 2] = 0xBA;
    sf->memory[0xBABE] = 0x23;
    process_line(sf);
    if (sf->accumulator != 0x53 || check_flags(sf->status, 0, 0, 2, 1, 2, 0, 1)) {
        return -1;
    }

    return 0;
}

static int INC_TEST(sf_t *sf) {
    sf->memory[ROM_START] = OP_INC | (ADDR_MODE_ZPG << 2);
    sf->memory[ROM_START + 1] = 0x65;
    sf->memory[0x65] = 0x80;
    process_line(sf);
    if (sf->memory[0x65] != 0x81 || check_flags(sf->status, 1, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->memory[ROM_START] = OP_INC | (ADDR_MODE_ABS << 2);
    sf->memory[ROM_START + 1] = 0xFF;
    sf->memory[ROM_START + 2] = 0xFF;
    sf->memory[0xFFFF] = 0x7F;
    process_line(sf);
    if (sf->memory[0xFFFF] != 0x80 || check_flags(sf->status, 1, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->x_index = 0x20; 
    sf->memory[ROM_START] = OP_INC | (ADDR_MODE_ZPG_X << 2);
    sf->memory[ROM_START + 1] = 0x90;
    sf->memory[0xB0] = 0xFF;
    process_line(sf);
    if (sf->memory[0xB0] != 0x00 || check_flags(sf->status, 0, 2, 2, 2, 2, 1, 2)) {
        return -1;
    }

    sf->pc = ROM_START;
    sf->x_index = 0x2B;
    sf->memory[ROM_START] = OP_INC | (ADDR_MODE_ABS_X << 2);
    sf->memory[ROM_START + 1] = 0xD3;
    sf->memory[ROM_START + 2] = 0xCA;
    sf->memory[0xCAFE] = 0x22;
    process_line(sf);
    if (sf->memory[0xCAFE] != 0x23 || check_flags(sf->status, 0, 2, 2, 2, 2, 0, 2)) {
        return -1;
    }

    return 0;
}

static int run_opcode_test(sf_t *sf, int (*test_ptr)(sf_t *), char *test_name) {
    initialize_regs(sf, ROM_START);

    if ((*test_ptr)(sf) < 0) {
        printf("%s failed\n", test_name);
        return -1;
    }
}

int run_opcode_tests(sf_t *sf) {
    assert(run_opcode_test(sf, BRK_RTI_TEST, "BRK_RTI_TEST") == 0);
    assert(run_opcode_test(sf, PHP_PLP_TEST, "PHP_PLP_TEST") == 0);
    assert(run_opcode_test(sf, BPL_TEST, "BPL_TEST") == 0);
    assert(run_opcode_test(sf, CLC_CLD_CLI_CLV_TEST, "CLC_CLD_CLI_CLV_TEST") == 0);
    assert(run_opcode_test(sf, JSR_RTS_TEST, "JSR_RTS_TEST") == 0);
    assert(run_opcode_test(sf, BMI_TEST, "BMI_TEST") == 0);
    assert(run_opcode_test(sf, SEC_SED_SEI_TEST, "SEC_SED_SEI_TEST") == 0);
    assert(run_opcode_test(sf, JMP_TEST, "JMP_TEST") == 0);
    assert(run_opcode_test(sf, BVC_TEST, "BVC_TEST") == 0);
    assert(run_opcode_test(sf, BVS_TEST, "BVS_TEST") == 0);
    assert(run_opcode_test(sf, JI_TEST, "JI_TEST") == 0);
    assert(run_opcode_test(sf, TXA_TYA_TEST, "TXA_TYA_TEST") == 0);
    assert(run_opcode_test(sf, BCC_TEST, "BCC_TEST") == 0);
    assert(run_opcode_test(sf, TXS_TEST, "TXS_TEST") == 0);
    assert(run_opcode_test(sf, TAX_TAY_TEST, "TAX_TAY_TEST") == 0);
    assert(run_opcode_test(sf, BCS_TEST, "BCS_TEST") == 0);
    assert(run_opcode_test(sf, TSX_TEST, "TSX_TEST") == 0);
    assert(run_opcode_test(sf, INX_INY_TEST, "INX_INY_TEST") == 0);
    assert(run_opcode_test(sf, DEX_DEY_TEST, "DEX_DEY_TEST") == 0);
    assert(run_opcode_test(sf, BNE_TEST, "BNE_TEST") == 0);
    assert(run_opcode_test(sf, NOP_TEST, "NOP_TEST") == 0);
    assert(run_opcode_test(sf, BEQ_TEST, "BEQ_TEST") == 0);
    assert(run_opcode_test(sf, ORA_TEST, "ORA_TEST") == 0);
    assert(run_opcode_test(sf, ASL_TEST, "ASL_TEST") == 0);
    assert(run_opcode_test(sf, BIT_TEST, "BIT_TEST") == 0);
    assert(run_opcode_test(sf, AND_TEST, "AND_TEST") == 0);
    assert(run_opcode_test(sf, ROL_TEST, "ROL_TEST") == 0);
    assert(run_opcode_test(sf, EOR_TEST, "EOR_TEST") == 0);
    assert(run_opcode_test(sf, LSR_TEST, "LSR_TEST") == 0);
    assert(run_opcode_test(sf, ADC_TEST, "ADC_TEST") == 0);
    assert(run_opcode_test(sf, ROR_TEST, "ROR_TEST") == 0);
    assert(run_opcode_test(sf, STY_TEST, "STY_TEST") == 0);
    assert(run_opcode_test(sf, STA_TEST, "STA_TEST") == 0);
    assert(run_opcode_test(sf, STX_TEST, "STX_TEST") == 0);
    assert(run_opcode_test(sf, LDY_TEST, "LDY_TEST") == 0);
    assert(run_opcode_test(sf, LDA_TEST, "LDA_TEST") == 0);
    assert(run_opcode_test(sf, LDX_TEST, "LDX_TEST") == 0);
    assert(run_opcode_test(sf, CPY_TEST, "CPY_TEST") == 0);
    assert(run_opcode_test(sf, CMP_TEST, "CMP_TEST") == 0);
    assert(run_opcode_test(sf, DEC_TEST, "DEC_TEST") == 0);
    assert(run_opcode_test(sf, CPX_TEST, "CPX_TEST") == 0);
    assert(run_opcode_test(sf, SBC_TEST, "SBC_TEST") == 0);
    assert(run_opcode_test(sf, INC_TEST, "INC_TEST") == 0);

    printf("ALL TESTS PASSED!\n");
    return 0;
}

/* DATA STRUCTURE TESTS */

int table_test() {
    Table_t *t = new_table(8);
    if (t->size != 8 || t->occupied != 0) {
        return -1;
    }
    
    add_to_table(&t, "test1", 0xBEEF);
    if (get_value(t, "test1") != 0xBEEF) {
        return -1;
    }
    
    add_to_table(&t, "test2", 0xDEAD);
    add_to_table(&t, "test3", 0xBABE);
    add_to_table(&t, "test4", 0xBABA);
    add_to_table(&t, "test5", 0xCACA);
    add_to_table(&t, "test6", 0xACDC);
    add_to_table(&t, "test7", 0xDEAF);

    if (t->size != 16 || t->occupied != 7) {
        return -1;
    }

    if (get_value(t, "test3") != 0xBABE) {
        return -1;
    }

    free_table(t);

    return 0;
}
