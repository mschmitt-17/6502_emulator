#ifndef     __GENERATOR_H
#define     __GENERATOR_H

#include <stdint.h>
#include "token.h"
#include "bytecode.h"
#include "table.h"

#define INVALID_OPCODE      0xFF

Program_t * clip_to_program(uint8_t *sf_asm, Clip_t *c, Table_t **label_table_dbl_ptr);

#endif