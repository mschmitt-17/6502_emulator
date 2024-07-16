#ifndef __ADDRESSING_JUMPTABLE_H
#define __ADDRESSING_JUMPTABLE_H

#include <stdint.h>

extern int (* ptr_arr[0xFF])(uint8_t opcode, uint8_t addressing_mode); 

#endif
