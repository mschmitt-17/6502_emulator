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

static int PHP_TEST(sf_t *sf) {
    sf->memory[ROM_START] = OP_PHP;
    sf->status = TEST_MAGIC;
    process_line(sf);
    if (sf->memory[sf->esp + 1] != TEST_MAGIC) {
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

static int CLC_TEST(sf_t *sf) {
    sf->memory[ROM_START] = OP_CLC;
    sf->status |= (1 << CARRY_INDEX);
    process_line(sf);
    if (sf->status & (1 << CARRY_INDEX)) {
        return -1;
    }
    return 0;
}

static int JSR_RTI_TEST(sf_t *sf) {
    int sub_address = 0xB0B0;
    sf->memory[ROM_START] = OP_JSR;
    sf->memory[ROM_START + 1] = sub_address & 0x00FF;
    sf->memory[ROM_START + 2] = (sub_address & 0xFF00) >> 8;
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

static int run_test(sf_t *sf, int (*test_ptr)(sf_t *), char *test_name) {
    initialize_regs(sf);

    if ((*test_ptr)(sf) < 0) {
        printf("%s failed\n", test_name);
        return -1;
    }
}

int run_tests(sf_t *sf) {
    assert(run_test(sf, BRK_RTI_TEST, "BRK_RTI_TEST") == 0);
    assert(run_test(sf, PHP_TEST, "PHP_TEST") == 0);
    assert(run_test(sf, BPL_TEST, "BPL_TEST") == 0);
    assert(run_test(sf, CLC_TEST, "CLC_TEST") == 0);
    assert(run_test(sf, JSR_RTI_TEST, "JSR_RTI_TEST") == 0);
    assert(run_test(sf, BMI_TEST, "BMI_TEST") == 0);
    printf("ALL TESTS PASSED!\n");
    return 0;
}
