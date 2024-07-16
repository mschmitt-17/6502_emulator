#ifndef __BYTECODE_H
#define __BYTECODE_H

#include <stdint.h>

#define BYTECODE_INIT_SIZE      2
#define BYTECODE_GROWTH_FACTOR  2

typedef struct {
    uint32_t size;
    uint8_t *start;
    uint32_t index;
} Bytecode_t; 

Bytecode_t* new_bytecode();
void free_bytecode(Bytecode_t *bc);
void add_to_bytecode(Bytecode_t *bc, uint8_t *write_buf, uint32_t num_bytes, uint32_t line_number);

#endif
