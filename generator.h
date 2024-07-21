#ifndef     __GENERATOR_H
#define     __GENERATOR_H

#include <stdint.h>
#include "bytecode.h"

#define INVALID_OPCODE      0xFF
#define INVALID_ADDR_MODE   0xFF

Program_t * clip_to_program(Program_t *p);

#endif