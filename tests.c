#include "stdio.h"
#include "tests.h"
#include "assert.h"

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
    unsigned char offset = 0xF6;
    sf->memory[ROM_START] = OP_BPL;
    sf->memory[ROM_START + 1] = offset;
    sf->status |= (1 << NEGATIVE_INDEX);
    // test when N = 1
    process_line(sf);
    if ((sf->pc == ROM_START + offset) || sf->pc != ROM_START + 2) {
        return -1;
    }
    sf->pc = ROM_START;
    sf->status &= (~(1 << NEGATIVE_INDEX));
    // test when N = 0
    process_line(sf);
    if (sf->pc != ROM_START + offset) {
        return -1;
    }
    return 0;
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
    unsigned char offset = 0xF6;
    sf->memory[ROM_START] = OP_BMI;
    sf->memory[ROM_START + 1] = offset;
    // test when N = 0
    process_line(sf);
    if ((sf->pc == ROM_START + offset) || sf->pc != ROM_START + 2) {
        return -1;
    }
    sf->pc = ROM_START;
    sf->status |= (1 << NEGATIVE_INDEX);
    // test when N = 1
    process_line(sf);
    if (sf->pc != ROM_START + offset) {
        return -1;
    }
    return 0;
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
    unsigned char offset = 0xF6;
    sf->memory[ROM_START] = OP_BVC;
    sf->memory[ROM_START + 1] = offset;
    // test when V = 1
    sf->status |= (1 << OVERFLOW_INDEX);
    process_line(sf);
    if ((sf->pc == ROM_START + offset) || sf->pc != ROM_START + 2) {
        return -1;
    }
    sf->pc = ROM_START;
    sf->status &= ~(1 << OVERFLOW_INDEX);
    // test when V = 0
    process_line(sf);
    if (sf->pc != ROM_START + offset) {
        return -1;
    }
    return 0;
}

static int BVS_TEST(sf_t *sf) {
    unsigned char offset = 0xF6;
    sf->memory[ROM_START] = OP_BVS;
    sf->memory[ROM_START + 1] = offset;
    // test when V = 0
    process_line(sf);
    if ((sf->pc == ROM_START + offset) || sf->pc != ROM_START + 2) {
        return -1;
    }
    sf->pc = ROM_START;
    sf->status |= (1 << OVERFLOW_INDEX);
    // test when V = 0
    process_line(sf);
    if (sf->pc != ROM_START + offset) {
        return -1;
    }
    return 0;
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
    printf("ALL TESTS PASSED!\n");
    return 0;
}
