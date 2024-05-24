#include "stdio.h"
#include "tests.h"
#include "assert.h"

// helper function for tests that should branch when flag is not set
static int branch_not_set_test(sf_t *sf, uint8_t opcode, uint8_t flag_index) {
    unsigned char offset = 0xF6;
    sf->memory[ROM_START] = opcode;
    sf->memory[ROM_START + 1] = offset;
    sf->status |= (1 << flag_index);
    // test when flag = 1
    process_line(sf);
    if ((sf->pc == ROM_START + offset) || sf->pc != ROM_START + 2) {
        return -1;
    }
    sf->pc = ROM_START;
    sf->status &= (~(1 << flag_index));
    // test when flag = 0
    process_line(sf);
    if (sf->pc != ROM_START + offset) {
        return -1;
    }
    return 0;
}

// helper function for tests that should branch when flag is set
static int branch_set_test(sf_t *sf, uint8_t opcode, uint8_t flag_index) {
    unsigned char offset = 0xF6;
    sf->memory[ROM_START] = opcode;
    sf->memory[ROM_START + 1] = offset;
    // test when flag = 0
    process_line(sf);
    if ((sf->pc == ROM_START + offset) || sf->pc != ROM_START + 2) {
        return -1;
    }
    sf->pc = ROM_START;
    sf->status |= (1 << flag_index);
    // test when flag = 1
    process_line(sf);
    if (sf->pc != ROM_START + offset) {
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
    sf->x_index = 0x12;
    sf->y_index = 0x34;
    process_line(sf);
    if (sf->accumulator != sf->x_index) {
        return -1;
    }
    process_line(sf);
    if (sf->accumulator != sf->y_index) {
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
    process_line(sf);
    if ((sf->x_index != sf->accumulator) || (sf->y_index != sf->accumulator)) {
        return -1;
    }
    return 0;
}

static int BCS_TEST(sf_t *sf) {
    branch_set_test(sf, OP_BCS, CARRY_INDEX);
}

static int TSX_TEST(sf_t *sf) {
    sf->memory[ROM_START] = OP_TSX;
    sf->esp = 0x12;
    process_line(sf);
    if (sf->x_index != sf->esp) {
        return -1;
    }
    return 0;
}

static int INX_INY_TEST(sf_t *sf) {
    sf->memory[ROM_START] = OP_INX;
    sf->memory[ROM_START + 1] = OP_INY;
    process_line(sf);
    process_line(sf);
    if ((sf->x_index != 1) || (sf->y_index != 1)) {
        return -1;
    }
    return 0;
}

static int DEX_DEY_TEST(sf_t *sf) {
    sf->memory[ROM_START] = OP_DEX;
    sf->memory[ROM_START + 1] = OP_DEY;
    process_line(sf);
    process_line(sf);
    if ((sf->x_index != 0xFF) || (sf->y_index != 0xFF)) { // unsigned values will overflow
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

static int run_test(sf_t *sf, int (*test_ptr)(sf_t *), char *test_name) {
    initialize_regs(sf);

    if ((*test_ptr)(sf) < 0) {
        printf("%s failed\n", test_name);
        return -1;
    }
}

int run_tests(sf_t *sf) {
    assert(run_test(sf, BRK_RTI_TEST, "BRK_RTI_TEST") == 0);
    assert(run_test(sf, PHP_PLP_TEST, "PHP_PLP_TEST") == 0);
    assert(run_test(sf, BPL_TEST, "BPL_TEST") == 0);
    assert(run_test(sf, CLC_CLD_CLI_CLV_TEST, "CLC_CLD_CLI_CLV_TEST") == 0);
    assert(run_test(sf, JSR_RTS_TEST, "JSR_RTS_TEST") == 0);
    assert(run_test(sf, BMI_TEST, "BMI_TEST") == 0);
    assert(run_test(sf, SEC_SED_SEI_TEST, "SEC_SED_SEI_TEST") == 0);
    assert(run_test(sf, JMP_TEST, "JMP_TEST") == 0);
    assert(run_test(sf, BVC_TEST, "BVC_TEST") == 0);
    assert(run_test(sf, BVS_TEST, "BVS_TEST") == 0);
    assert(run_test(sf, JI_TEST, "JI_TEST") == 0);
    assert(run_test(sf, TXA_TYA_TEST, "TXA_TYA_TEST") == 0);
    assert(run_test(sf, BCC_TEST, "BCC_TEST") == 0);
    assert(run_test(sf, TXS_TEST, "TXS_TEST") == 0);
    assert(run_test(sf, TAX_TAY_TEST, "TAX_TAY_TEST") == 0);
    assert(run_test(sf, BCS_TEST, "BCS_TEST") == 0);
    assert(run_test(sf, TSX_TEST, "TSX_TEST") == 0);
    assert(run_test(sf, INX_INY_TEST, "INX_INY_TEST") == 0);
    assert(run_test(sf, DEX_DEY_TEST, "DEX_DEY_TEST") == 0);
    assert(run_test(sf, BNE_TEST, "BNE_TEST") == 0);
    assert(run_test(sf, NOP_TEST, "NOP_TEST") == 0);
    assert(run_test(sf, BEQ_TEST, "BEQ_TEST") == 0);

    printf("ALL TESTS PASSED!\n");
    return 0;
}
