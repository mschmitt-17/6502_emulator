#ifndef __LIB_H
#define __LIB_H

#include <stdint.h>

/* ERROR CODES */
#define ERR_NO_MEM                      0x01
#define ERR_INVALID_OPERAND_OPCODE      0x02
#define ERR_INVALID_ADDRESSING_MODE     0x03
#define ERR_FILE_NOOPEN                 0x04
#define ERR_SYNTAX                      0x05
#define ERR_INVALID_DIRECTIVE           0x06

uint8_t is_letter(uint8_t curr_char);
uint8_t is_decimal_number(uint8_t curr_char);
uint8_t is_hex_number(uint8_t curr_char);
uint8_t char_to_hex(uint8_t curr_char);

#endif
