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
#define ERR_EARLY_REF                   0x07
#define ERR_OVERFLOW                    0x08
#define ERR_NO_FILE                     0x09
#define ERR_INVALID_LABEL               0x0A
#define ERR_LABEL_ADDRESSING            0x0B

/*
 * 6502 memory map according to ChatGPT:
 *      0x0000 - 0x00FF: Zero Page
 *      0x0100 - 0x01FF: Stack
 *      0x0200 - 0x07FF: System Area
 *      0x8000 - 0xBFFF: Cartridge ROM
 *      0xC000 - 0xFFFF: Cartridge RAM
 */
#define ZERO_PAGE_START                 (0x0000)
#define STACK_START                     (0x01FF)
#define SYSTEM_START                    (0x0200)
#define ROM_START                       (0x8000)
#define RAM_START                       (0xC000)

uint8_t is_letter(uint8_t curr_char);
uint8_t is_decimal_number(uint8_t curr_char);
uint8_t is_hex_number(uint8_t curr_char);
uint8_t char_to_hex(uint8_t curr_char);
uint8_t *read_file(const char *file_path);
uint8_t validate_label(uint8_t *curr_char_ptr, uint8_t token_len);

#endif
