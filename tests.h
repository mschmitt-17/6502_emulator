#ifndef TESTS_H
#define TESTS_H

#include "6502.h"

#define TEST_MAGIC  0b10101100
#define MAGIC_ONE   0x12
#define MAGIC_TWO   0x34
#define MAGIC_THREE 0x56
#define MAGIC_FOUR  0x78
#define MAGIC_FIVE  0x90
#define MAGIC_SIZ   0x09

int run_tests(sf_t *sf);

#endif
