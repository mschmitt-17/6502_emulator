#ifndef     ASSEMBLER_H
#define     ASSEMBLER_H

#include <stdint.h>
#include "bytecode.h"

#define INVALID_OPCODE      0xFF
#define INVALID_ADDR_MODE   0xFF

Bytecode_t * assembly_to_bytecode(const char *file_path);

#endif      /* ASSEMBLER_H */