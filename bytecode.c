#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bytecode.h"
#include "error.h"

Bytecode_t* new_bytecode() {
    Bytecode_t *bc = (Bytecode_t *)malloc(sizeof(Bytecode_t));
    bc->start = (uint8_t *)malloc(BYTECODE_INIT_SIZE);
    bc->index = 0;
    bc->size = BYTECODE_INIT_SIZE;
    return bc;
}

void free_bytecode(Bytecode_t *bc) {
    free(bc->start);
    free(bc);
}

static uint8_t expand_bytecode(Bytecode_t *bc) {
    bc->start = (uint8_t *)realloc(bc->start, bc->size * BYTECODE_GROWTH_FACTOR);
    if (bc->start == NULL) {
        return -1;
    }
    bc->size *= BYTECODE_GROWTH_FACTOR;
    return 0;
}

void add_to_bytecode(Bytecode_t *bc, uint8_t *write_buf, uint32_t num_bytes, uint32_t line_number) {
    if ((num_bytes + bc->index) >= bc->size) {
        if (expand_bytecode(bc) == -1) {
            fprintf(stderr, "Error at line %d: bytecode memory allocation failed", line_number);
            exit(ERR_NO_MEM);
        }
    }
    memcpy(bc->start + bc->index, write_buf, num_bytes);
    bc->index += num_bytes;
}
