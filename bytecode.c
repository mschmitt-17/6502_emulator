#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib.h"
#include "bytecode.h"

#define BYTECODE_GROWTH_FACTOR  2
#define PROGRAM_GROWTH_FACTOR   2
#define BYTECODE_INIT_SIZE      256

void init_bytecode(Bytecode_t *bc, uint32_t size) {
    bc->start = (uint8_t *)malloc(size);
    bc->index = 0;
    bc->size = size;
}

static int expand_bytecode(Bytecode_t *bc) {
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

Program_t* new_program(uint32_t size) {
    Program_t *p = (Program_t *)malloc(sizeof(Program_t));
    p->start = (Bytecode_t *)malloc(size * sizeof(Bytecode_t));
    p->index = 0;
    p->size = size;
    return p;
}

void free_program(Program_t *p) {
    for (int i = 0; i < p->index; i++) {
        free(p->start[i].start);
    }
    free(p->start);
    free(p);
}

static int expand_program(Program_t *p) {
    p->start = (Program_t *)realloc(p->start, p->size * PROGRAM_GROWTH_FACTOR);
    if (p->start == NULL) {
        return -1;
    }
    p->size *= PROGRAM_GROWTH_FACTOR;
    return 0;
}

void open_bytecode(Program_t *p, uint16_t start_address, uint32_t line_number) {
    if (p->index >= p->size) {
        if (expand_program(p) == -1) {
            fprintf(stderr, "Error at line %d: program memory allocation failed", line_number);
            exit(ERR_NO_MEM);
        }
    }
    init_bytecode(p->start + p->index, BYTECODE_INIT_SIZE);
    p->index++;
}
