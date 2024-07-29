#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tests.h"
#include "lib.h"
#include "generator.h"
#include "scanner.h"

#define TABLE_INIT_SIZE         256

// #define RUN_TESTS

int main(int argc, char* argv[]) {
    sf_t *sf = (sf_t *)malloc(sizeof(sf_t));
#ifdef RUN_TESTS
    run_opcode_tests(sf);
    table_test();
#else
    if (argc == 1) {
        fprintf(stderr, "Error: must enter an assembly file to run\n");
        exit(ERR_NO_FILE);
    }

    uint8_t *sf_asm = read_file(argv[1]);
    Table_t *label_table = new_table(TABLE_INIT_SIZE);

    Clip_t *c = assembly_to_clip(sf_asm, &label_table);
    Program_t *p = clip_to_program(sf_asm, c, &label_table);
    
    for (int i = 0; i < p->index; i++) {
        load_bytecode(sf, p->start + i, p->start[i].load_address, p->start[i].index);
    }

    initialize_regs(sf, p->start[0].load_address);

    free_clip(c);
    free_program(p);
    free_table(label_table);
    free(sf_asm);

    for (int i = 0; i < 99; i++) {
        process_line(sf);
    }
#endif
    
    free(sf);
    return 0;
}
