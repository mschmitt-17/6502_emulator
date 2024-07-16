#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tests.h"
#include "assembler.h"

#define RUN_TESTS

int main() {
    sf_t *sf = (sf_t *)malloc(sizeof(sf_t));

#ifdef RUN_TESTS
    run_opcode_tests(sf);
    table_test();
#else
    Bytecode_t * bc = assembly_to_bytecode("test.txt");
    initialize_regs(sf);
    load_bytecode(sf, bc, ROM_START, bc->index);
    for (int i = 0; i < 99; i++) {
        process_line(sf);
    }
    free_bytecode(bc);
#endif
    
    free(sf);
    return 0;
}
