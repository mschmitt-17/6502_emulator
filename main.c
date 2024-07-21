#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tests.h"
#include "generator.h"
#include "scanner.h"

int main() {
    sf_t *sf = (sf_t *)malloc(sizeof(sf_t));

#ifdef RUN_TESTS
    run_opcode_tests(sf);
    table_test();
#else
    Clip_t *c = assembly_to_clip("scan_test.txt");
    initialize_regs(sf);
    //load_bytecode(sf, bc, ROM_START, bc->index);
    for (int i = 0; i < 99; i++) {
        process_line(sf);
    }
    //free_bytecode(bc);
#endif
    
    free(sf);
    return 0;
}
