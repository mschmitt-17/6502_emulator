#ifndef __BYTECODE_H
#define __BYTECODE_H

#include <stdint.h>

typedef struct {
    uint8_t *start;
    uint32_t size;
    uint32_t index;
} Bytecode_t; 

typedef struct {
    Bytecode_t *start;
    uint32_t size;
    uint32_t index;
} Program_t;

void add_to_bytecode(Bytecode_t *bc, uint8_t *write_buf, uint32_t num_bytes, uint32_t line_number);
Program_t* new_program(uint32_t size);
void free_program(Program_t *p);
void open_bytecode(Program_t *p, uint16_t *start_address, uint32_t line_number);

#endif
