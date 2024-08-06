#ifndef __SCANNER_H
#define __SCANNER_H

#define DIRECTIVE_ORG   0x01
#define DIRECTIVE_ERROR 0xFF

#include "6502.h"
#include "token.h"
#include "table.h"

Clip_t *assembly_to_clip(sf_t *sf, uint8_t *sf_asm, Table_t **label_table_dbl_ptr);

#endif
