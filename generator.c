#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib.h"
#include "generator.h"
#include "6502.h"
#include "addressing_jumptable.h"
#include "table.h"
#include "token.h"

#define TABLE_INIT_SIZE         256
#define PROGRAM_INIT_SIZE       256

/* skip_whitespace
 *      DESCRIPTION: helper function used to advance passed pointer beyond spaces and tabs
 *      INPUTS: curr_char_dbl_ptr -- double pointer to advance past spaces/tabs
 *      OUTPUTS: none
 *      SIDE EFFECTS: increments passed pointer beyond spaces/tabs
 */
static void skip_whitespace(uint8_t **curr_char_dbl_ptr) {
    while ((**curr_char_dbl_ptr == ' ') || (**curr_char_dbl_ptr == '\t')) {
        (*curr_char_dbl_ptr)++;
    }
}

/* next_line
 *      DESCRIPTION: helper function used to advance passed pointer to end of line
 *      INPUTS: curr_char_dbl_ptr -- double pointer to advance to end of line
 *      OUTPUTS: none
 *      SIDE EFFECTS: increments passed pointer to end of line
 */
static void next_line(uint8_t **curr_char_dbl_ptr) {
    while ((**curr_char_dbl_ptr != '\n') && (**curr_char_dbl_ptr != '\0')) {
        (*curr_char_dbl_ptr)++;
    }
}

/* read_hex_characters
 *      DESCRIPTION: advances passed pointer beyond hex characters
 *      INPUTS: curr_char_dbl_ptr -- pointer to be advanced beyond hex characters
 *      OUTPUTS: num_hex_characters -- the number of hex characters encountered
 *      SIDE EFFECTS: increments passed pointer beyond hex characters
 */
static uint32_t read_hex_characters(uint8_t **curr_char_dbl_ptr) {
    uint32_t num_hex_characters = 0;
    while (is_hex_number(**curr_char_dbl_ptr)) {
        num_hex_characters++;
        (*curr_char_dbl_ptr)++;
    }
    return num_hex_characters;
}

/* read_file
 *      DESCRIPTION: reads passed file into buffer
 *      INPUTS: file_path -- path to file to be read into buffer
 *      OUTPUTS: buffer which file has been read into
 *      SIDE EFFECTS: allocates memory for buffer which file is read into
 */
uint8_t *read_file(const char *file_path) {
    FILE *fp = fopen(file_path, "r");

    if (fp == NULL) {
        fprintf(stderr, "Could not open file %s\n", file_path);
        exit(ERR_FILE_NOOPEN);
    }

    // read to end of file to determine size of file
    fseek(fp, 0, SEEK_END);
    uint32_t file_size_in_bytes = ftell(fp);
    uint8_t *sf_asm = (uint8_t *)malloc(file_size_in_bytes + 1);
    rewind(fp);

    fread(sf_asm, 1, file_size_in_bytes, fp);
    sf_asm[file_size_in_bytes] = '\0'; // null-terminate program

    fclose(fp);

    return sf_asm;
}

/* determine_opcode
 *      DESCRIPTION: determines opcode of line passed pointer is on
 *      INPUTS: curr_char_dbl_ptr -- pointer to assembly being processed
 *      OUTPUTS: opcode of line on success, 0xFF on failure
 *      SIDE EFFECTS: increments passed buffer beyond opcode
 */
static uint8_t determine_opcode(uint8_t *curr_char_ptr) {
    switch (*curr_char_ptr) {
        case 'A':
            switch (*(++curr_char_ptr)) {
                case 'D':
                    if (*(++curr_char_ptr) == 'C') {
                        return OP_ADC;
                    }
                    break;
                case 'N':
                    if (*(++curr_char_ptr) == 'D') {
                        return OP_AND;
                    }
                    break;
                case 'S':
                    if (*(++curr_char_ptr) == 'L') {
                        return OP_ASL;
                    }
                    break;
                default:
                    break;
            }
            break;

        case 'B':
            switch (*(++curr_char_ptr)) {
                case 'C':
                    curr_char_ptr++;
                    if (*curr_char_ptr == 'C') {
                        return OP_BCC;
                    } else if (*curr_char_ptr == 'S') {
                        return OP_BCS;
                    }
                    break;
                case 'E':
                    if (*(++curr_char_ptr) == 'Q') {
                        return OP_BEQ;
                    }
                    break;
                case 'I':
                    if (*(++curr_char_ptr) == 'T') {
                        return OP_BIT;
                    }
                    break;
                case 'M':
                    if (*(++curr_char_ptr) == 'I') {
                        return OP_BMI;
                    }
                    break;
                case 'N':
                    if (*(++curr_char_ptr) == 'E') {
                        return OP_BNE;
                    }
                    break;
                case 'P':
                    if (*(++curr_char_ptr) == 'L') {
                        return OP_BPL;
                    }
                    break;
                case 'R':
                    if (*(++curr_char_ptr) == 'K') {
                        return OP_BRK;
                    }
                    break;
                case 'V':
                    curr_char_ptr++;
                    if (*curr_char_ptr == 'C') {
                        return OP_BVC;
                    } else if (*curr_char_ptr == 'S') {
                        return OP_BVS;
                    }
                    break;
                default:
                    break;
            }
            break;
        
        case 'C':
            switch (*(++curr_char_ptr)) {
                case 'L':
                    switch (*(++curr_char_ptr)) {
                        case 'C':
                            return OP_CLC;
                        case 'D':
                            return OP_CLD;
                        case 'I':
                            return OP_CLI;
                        case 'V':
                            return OP_CLV;
                        default:
                            break;
                    }
                    break;
                case 'M':
                    if (*(++curr_char_ptr) == 'P') {
                        return OP_CMP;
                    }
                    break;
                case 'P':
                    curr_char_ptr++;
                    if (*curr_char_ptr == 'X') {
                        return OP_CPX;
                    } else if (*curr_char_ptr == 'Y') {
                        return OP_CPY;
                    }
                    break;
                default:
                    break;
            }
            break;
        
        case 'D':
            if (*(++curr_char_ptr) == 'E') {
                switch (*(++curr_char_ptr)) {
                    case 'C':
                        return OP_DEC;
                    case 'X':
                        return OP_DEX;
                    case 'Y':
                        return OP_DEY;
                    default:
                        break;
                }
            }
            break;
        
        case 'E':
            if (*(++curr_char_ptr) == 'O') {
                if (*(++curr_char_ptr) == 'R') {
                    return OP_EOR;
                }
            }
            break;
        
        case 'I':
            if (*(++curr_char_ptr) == 'N') {
                switch (*(++curr_char_ptr)) {
                    case 'C':
                        return OP_INC;
                    case 'X':
                        return OP_INX;
                    case 'Y':
                        return OP_INY;
                    default:
                        break;
                }
            }
            break;
        
        case 'J':
            curr_char_ptr++;
            if (*curr_char_ptr == 'M') {
                if (*(++curr_char_ptr) == 'P') {
                    return OP_JMP;
                }
            } else if (*curr_char_ptr == 'S') {
                if (*(++curr_char_ptr) == 'R') {
                    return OP_JSR_GEN;
                }
            }
            break;
        
        case 'L':
            curr_char_ptr++;
            if (*curr_char_ptr == 'D') {
                switch(*(++curr_char_ptr)) {
                    case 'A':
                        return OP_LDA;
                        break;
                    case 'X':
                        return OP_LDX;
                        break;
                    case 'Y':
                        return OP_LDY;
                        break;
                    default:
                        break;
                }
            } else if (*curr_char_ptr == 'S') {
                if (*(++curr_char_ptr) == 'R') {
                    return OP_LSR;
                }
            }
            break;

        case 'N':
            if (*(++curr_char_ptr) == 'O') {
                if (*(++curr_char_ptr) == 'P') {
                    return OP_NOP;
                }
            }
            break;
        
        case 'O':
            if (*(++curr_char_ptr) == 'R') {
                if (*(++curr_char_ptr) == 'A') {
                    return OP_ORA;
                }
            }
            break;
        
        case 'P':
            curr_char_ptr++;
            if (*curr_char_ptr == 'H') {
                curr_char_ptr++;
                if (*curr_char_ptr == 'A') {
                    return OP_PHA;
                } else if (*curr_char_ptr == 'P') {
                    return OP_PHP;
                }
            } else if (*curr_char_ptr == 'L') {
                curr_char_ptr++;
                if (*curr_char_ptr == 'A') {
                    return OP_PLA;
                } else if (*curr_char_ptr == 'P') {
                    return OP_PLP;
                }
            }
            break;
        
        case 'R':
            curr_char_ptr++;
            if (*curr_char_ptr == 'O') {
                curr_char_ptr++;
                if (*curr_char_ptr == 'L') {
                    return OP_ROL;
                } else if (*curr_char_ptr == 'R') {
                    return OP_ROR;
                }
            } else if (*curr_char_ptr == 'T') {
                curr_char_ptr++;
                if (*curr_char_ptr == 'I') {
                    return OP_RTI;
                } else if (*curr_char_ptr == 'S') {
                    return OP_RTS;
                }
            }
            break;
        
        case 'S':
            switch (*(++curr_char_ptr)) {
                case 'B':
                    if (*(++curr_char_ptr) == 'C') {
                        return OP_SBC;
                    }
                    break;
                case 'E':
                    switch (*(++curr_char_ptr)) {
                        case 'C':
                            return OP_SEC;
                        case 'D':
                            return OP_SED;
                        case 'I':
                            return OP_SEI;
                        default:
                            break;
                    }
                    break;
                case 'T':
                    switch (*(++curr_char_ptr)) {
                        case 'A':
                            return OP_STA;
                        case 'X':
                            return OP_STX;
                        case 'Y':
                            return OP_STY;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
            break;
        
        case 'T':
            switch (*(++curr_char_ptr)) {
                case 'A':
                    curr_char_ptr++;
                    if (*curr_char_ptr == 'X') {
                        return OP_TAX;
                    } else if (*curr_char_ptr == 'Y') {
                        return OP_TAY;
                    }
                    break;
                case 'S':
                    if (*(++curr_char_ptr) == 'X') {
                        return OP_TSX;
                    }
                    break;
                case 'X':
                    curr_char_ptr++;
                    if (*curr_char_ptr == 'A') {
                        return OP_TXA;
                    } else if (*curr_char_ptr == 'S') {
                        return OP_TXS;
                    }
                    break;
                case 'Y':
                    if (*(++curr_char_ptr) == 'A') {
                        return OP_TYA;
                    }
                    break;
                default:
                    break;
            }
            break;
    
        default:
            break;
    }

    return INVALID_OPCODE; // placeholder invalid opcode
}

/* determine_operand_and_addressing
 *      DESCRIPTION: determines operand and addressing mode based on operand
 *      INPUTS: curr_char_dbl_ptr -- pointer to assembly being processed
 *              return_buf -- buffer with at least 3 bytes of allocated memory
 *      OUTPUTS: none
 *      SIDE EFFECTS: fills return_buf with return_buf[0] = addressing mode, return_buf[1] = low byte of operand, return_buf[2] = high byte of operand
 */
static void determine_operand_and_addressing(uint8_t *curr_char_ptr, uint8_t token_len, uint8_t *return_buf) {
    switch (token_len) {
        case 1:
            // OPC A -- accumulator addressing mode
            if (*curr_char_ptr == 'A' || *curr_char_ptr == 'a') {
                return_buf[0] = ADDR_MODE_ACCUM_GEN;
            }
            break;
        case 3:
            // OPC $LL -- zero-page addressing mode
            if (*curr_char_ptr == '$') {
                if (is_hex_number(*(curr_char_ptr + 1))) {
                    if (is_hex_number(*(curr_char_ptr + 2))) {
                        return_buf[0] = ADDR_MODE_ZPG;
                        return_buf[1] = (char_to_hex((*(curr_char_ptr + 1)) << 4) | char_to_hex(*(curr_char_ptr + 2)));
                    }
                }
            }
            break;
        case 4:
            // OPC #$BB -- immediate addressing mode
            if (*curr_char_ptr == '#') {
                if (*(curr_char_ptr + 1) == '$') {
                    if (is_hex_number(*(curr_char_ptr + 2))) {
                        if (is_hex_number(*(curr_char_ptr + 3))) {
                            return_buf[0] = ADDR_MODE_IMM;
                            return_buf[1] = (char_to_hex((*(curr_char_ptr + 2)) << 4) | char_to_hex(*(curr_char_ptr + 3)));
                        }
                    }
                }
            }
            break;
        case 5:
            if (*curr_char_ptr == '$') {
                if (is_hex_number(*(curr_char_ptr + 1))) {
                    if (is_hex_number(*(curr_char_ptr + 2))) {
                        if (is_hex_number(*(curr_char_ptr + 3))) {
                            if (is_hex_number(*(curr_char_ptr + 4))) {
                                // OPC $HHLL -- absolute addressing mode
                                return_buf[0] = ADDR_MODE_ABS;
                                return_buf[1] = (char_to_hex(*(curr_char_ptr + 3)) << 4) | char_to_hex(*(curr_char_ptr + 4));
                                return_buf[2] = (char_to_hex(*(curr_char_ptr + 1)) << 4) | char_to_hex(*(curr_char_ptr + 2));
                            }
                        } else if (*(curr_char_ptr + 3) == ',') {
                            if (*(curr_char_ptr + 4) == 'X' || *(curr_char_ptr + 4) == 'x') {
                                // OPC $LL,X -- zero-page x-indexed addressing mode
                                return_buf[0] = ADDR_MODE_ZPG_X;
                                return_buf[1] = (char_to_hex((*(curr_char_ptr + 1)) << 4) | char_to_hex(*(curr_char_ptr + 2)));
                            } else if (*(curr_char_ptr + 4) == 'Y' || *(curr_char_ptr + 4) == 'y') {
                                // OPC $LL,Y -- zero-page y-indexed addressing mode
                                return_buf[0] = ADDR_MODE_ZPG_Y;
                                return_buf[1] = (char_to_hex((*(curr_char_ptr + 1)) << 4) | char_to_hex(*(curr_char_ptr + 2)));
                            }
                        }
                    }
                }
            }
            break;
        case 7:
            if (*curr_char_ptr == '(') {
                if (*(curr_char_ptr + 1) == '$') {
                    if (is_hex_number(*(curr_char_ptr + 2))) {
                        if (is_hex_number(*(curr_char_ptr + 3))) {
                            if (is_hex_number(*(curr_char_ptr + 4))) {
                                if (is_hex_number(*(curr_char_ptr + 5))) {
                                    if (*(curr_char_ptr + 6) == ')') {
                                        //OPC ($HHLL) -- indirect addressing mode (branch only)
                                        return_buf[0] = ADDR_MODE_IND;
                                        return_buf[1] = (char_to_hex((*(curr_char_ptr + 4)) << 4) | char_to_hex(*(curr_char_ptr + 5)));
                                        return_buf[2] = (char_to_hex((*(curr_char_ptr + 2)) << 4) | char_to_hex(*(curr_char_ptr + 3)));
                                    }
                                }
                            } else if (*(curr_char_ptr + 4) == ',') {
                                if (*(curr_char_ptr + 5) == 'X') {
                                    if (*(curr_char_ptr + 6) == ')') {
                                        // OPC ($LL,X) -- indirect x-indexed addressing mode
                                        return_buf[0] = ADDR_MODE_IND_X;
                                        return_buf[1] = (char_to_hex((*(curr_char_ptr + 2)) << 4) | char_to_hex(*(curr_char_ptr + 3)));
                                    }
                                }
                            } else if (*(curr_char_ptr + 4) == ')') {
                                if (*(curr_char_ptr + 5) == ',') {
                                    if (*(curr_char_ptr + 6) == 'Y') {
                                        // OPC ($LL),Y -- indirect y-indexed addressing mode
                                        return_buf[0] = ADDR_MODE_IND_Y;
                                        return_buf[1] = (char_to_hex((*(curr_char_ptr + 2)) << 4) | char_to_hex(*(curr_char_ptr + 3)));
                                    }
                                }
                            }
                        }
                    }
                }
            } else if (*curr_char_ptr == '$') {
                if (is_hex_number(*(curr_char_ptr + 1))) {
                    if (is_hex_number(*(curr_char_ptr + 2))) {
                        if (is_hex_number(*(curr_char_ptr + 3))) {
                            if (is_hex_number(*(curr_char_ptr + 4))) {
                                if (*(curr_char_ptr + 5) == ',') {
                                    if (*(curr_char_ptr + 6) == 'X' || *(curr_char_ptr + 6) == 'x') {
                                        // OPC $HHLL,X -- absolute x-indexed addressing mode
                                        return_buf[0] = ADDR_MODE_ABS_X;
                                        return_buf[1] = (char_to_hex((*(curr_char_ptr + 3)) << 4) | char_to_hex(*(curr_char_ptr + 4)));
                                        return_buf[2] = (char_to_hex((*(curr_char_ptr + 1)) << 4) | char_to_hex(*(curr_char_ptr + 2)));
                                    } else if (*(curr_char_ptr + 6) == 'Y' || *(curr_char_ptr + 6) == 'y') {
                                        // OPC $HHLL,Y -- absolute x-indexed addressing mode
                                        return_buf[0] = ADDR_MODE_ABS_Y;
                                        return_buf[1] = (char_to_hex((*(curr_char_ptr + 3)) << 4) | char_to_hex(*(curr_char_ptr + 4)));
                                        return_buf[2] = (char_to_hex((*(curr_char_ptr + 1)) << 4) | char_to_hex(*(curr_char_ptr + 2)));
                                    }
                                }
                            }
                        }
                    }
                }
            }
            break;
        default:
            break;
    }
    // switch (*curr_char_ptr) {
    //     case 'A':
    //         // OPC A -- Accumulator Addressing Mode
    //         return_buf[0] = ADDR_MODE_ACCUM_PARSE;
    //         break;
    //     case '$':
    //         (*curr_char_dbl_ptr)++;
    //         uint32_t num_hex_characters = read_hex_characters(curr_char_dbl_ptr);
    //         if (num_hex_characters == 2) {
    //             if (**curr_char_dbl_ptr == ',') {
    //                 (*curr_char_dbl_ptr)++;
    //                 if (**curr_char_dbl_ptr == 'X' || **curr_char_dbl_ptr == 'x') {
    //                     // OPC $LL,X -- Zero-Page X-Indexed Addressing Mode
    //                     (*curr_char_dbl_ptr)++;
    //                     return_buf[0] = ADDR_MODE_ZPG_X;
    //                     return_buf[1] = (char_to_hex(*((*curr_char_dbl_ptr) - 4)) << 4) | char_to_hex(*((*curr_char_dbl_ptr) - 3));
    //                 } else if (**curr_char_dbl_ptr == 'Y' || **curr_char_dbl_ptr == 'y') {
    //                     // OPC $LL,Y -- Zero-Page Y-Indexed Addressing Mode
    //                     (*curr_char_dbl_ptr)++;
    //                     return_buf[0] = ADDR_MODE_ZPG_Y_PARSE;
    //                     return_buf[1] = (char_to_hex(*((*curr_char_dbl_ptr) - 4)) << 4) | char_to_hex(*((*curr_char_dbl_ptr) - 3));
    //                 }
    //             } else {
    //                 // OPC $LL -- Zero-Page Addressing Mode (Relative addressing depends on instruction)
    //                 return_buf[0] = ADDR_MODE_ZPG;
    //                 return_buf[1] = (char_to_hex(*((*curr_char_dbl_ptr) - 2)) << 4) | char_to_hex(*((*curr_char_dbl_ptr) - 1));
    //             }
    //         } else if (num_hex_characters == 4) {
    //             if (**curr_char_dbl_ptr == ',') {
    //                 (*curr_char_dbl_ptr)++;
    //                 if (**curr_char_dbl_ptr == 'X' || **curr_char_dbl_ptr == 'x') {
    //                     // OPC $HHLL,X -- Absolute X-Indexed Addressing Mode
    //                     (*curr_char_dbl_ptr)++;
    //                     return_buf[0] = ADDR_MODE_ABS_X;
    //                     return_buf[1] = (char_to_hex(*((*curr_char_dbl_ptr) - 4)) << 4) | char_to_hex(*((*curr_char_dbl_ptr) - 3));
    //                     return_buf[2] = (char_to_hex(*((*curr_char_dbl_ptr) - 6)) << 4) | char_to_hex(*((*curr_char_dbl_ptr) - 5));
    //                 } else if (**curr_char_dbl_ptr == 'Y' || **curr_char_dbl_ptr == 'y') {
    //                     // OPC $HHLL,Y -- Absolute Y-Indexed Addressing Mode
    //                     (*curr_char_dbl_ptr)++;
    //                     return_buf[0] = ADDR_MODE_ABS_Y;
    //                     return_buf[1] = (char_to_hex(*((*curr_char_dbl_ptr) - 4)) << 4) | char_to_hex(*((*curr_char_dbl_ptr) - 3));
    //                     return_buf[2] = (char_to_hex(*((*curr_char_dbl_ptr) - 6)) << 4) | char_to_hex(*((*curr_char_dbl_ptr) - 5));
    //                 }
    //             } else {
    //                 // OPC $HHLL -- Absolute Addressing Mode
    //                 return_buf[0] = ADDR_MODE_ABS;
    //                 return_buf[1] = (char_to_hex(*((*curr_char_dbl_ptr) - 2)) << 4) | char_to_hex(*((*curr_char_dbl_ptr) - 1));
    //                 return_buf[2] = (char_to_hex(*((*curr_char_dbl_ptr) - 4)) << 4) | char_to_hex(*((*curr_char_dbl_ptr) - 3));
    //             }
    //         }
    //         break;
    //     case '(':
    //         (*curr_char_dbl_ptr)++;
    //         if (**curr_char_dbl_ptr == '$') {
    //             (*curr_char_dbl_ptr)++;
    //             uint32_t num_hex_characters = read_hex_characters(curr_char_dbl_ptr);
    //             if (num_hex_characters == 2) {
    //                 if (**curr_char_dbl_ptr == ')') {
    //                     (*curr_char_dbl_ptr)++;
    //                     if (**curr_char_dbl_ptr == ',') {
    //                         (*curr_char_dbl_ptr)++;
    //                         if (**curr_char_dbl_ptr == 'Y') {
    //                             // OPC ($LL),Y -- Indirect Y-Indexed Addressing Mode
    //                             (*curr_char_dbl_ptr)++;
    //                             return_buf[0] = ADDR_MODE_IND_Y;
    //                             return_buf[1] = (char_to_hex(*((*curr_char_dbl_ptr) - 5)) << 4) | char_to_hex(*((*curr_char_dbl_ptr) - 4));
    //                         }
    //                     }   
    //                 } else if (**curr_char_dbl_ptr == ',') {
    //                     (*curr_char_dbl_ptr)++;
    //                     if (**curr_char_dbl_ptr == 'X' || **curr_char_dbl_ptr == 'x') {
    //                         (*curr_char_dbl_ptr)++;
    //                         if (**curr_char_dbl_ptr == ')') {
    //                             // OPC ($LL,X) -- Indirect X-Indexed Addressing Mode
    //                             (*curr_char_dbl_ptr)++;
    //                             return_buf[0] = ADDR_MODE_IND_X;
    //                             return_buf[1] = (char_to_hex(*((*curr_char_dbl_ptr) - 5)) << 4) | char_to_hex(*((*curr_char_dbl_ptr) - 4));
    //                         }
    //                     }
    //                 }
    //             } else if (num_hex_characters == 4) {
    //                 if (**curr_char_dbl_ptr == ')') {
    //                     //OPC ($HHLL) -- Indirect Addressing Mode (branch only)
    //                     (*curr_char_dbl_ptr)++;
    //                     return_buf[0] = ADDR_MODE_IND;
    //                     return_buf[1] = (char_to_hex(*((*curr_char_dbl_ptr) - 3)) << 4) | char_to_hex(*((*curr_char_dbl_ptr) - 2));
    //                     return_buf[2] = (char_to_hex(*((*curr_char_dbl_ptr) - 5)) << 4) | char_to_hex(*((*curr_char_dbl_ptr) - 4));
    //                 }
    //             }
    //         }
    //         break;
    //     case '#':
    //         (*curr_char_dbl_ptr)++;
    //         if (**curr_char_dbl_ptr == '$') {
    //             (*curr_char_dbl_ptr)++;
    //             uint32_t num_hex_characters = read_hex_characters(curr_char_dbl_ptr);
    //             if (num_hex_characters == 2) {
    //                 // OPC #$BB -- Immediate Addressing Mode
    //                 return_buf[0] = ADDR_MODE_IMM;
    //                 return_buf[1] = (char_to_hex(*((*curr_char_dbl_ptr) - 2)) << 4) | char_to_hex(*((*curr_char_dbl_ptr) - 1));
    //             }
    //         }
    //         break;
    //     case '\n':
    //     case ';': // comment
    //     case '\0': // end of program
    //         // OPC -- Implied Addressing Mode
    //         return_buf[0] = ADDR_MODE_IMP;
    //         break;
    //     default:
    //         break;
    // }
}

/* parse_line
 *      DESCRIPTION: determines opcode and operand for current line and adds to passed return buffer 
 *      INPUTS: curr_char_dbl_ptr -- pointer to assembly being processed
 *              line_number -- number corresponding to line being processed
 *              return_buf -- buffer to write opcode and operand to
 *      OUTPUTS: none
 *      SIDE EFFECTS: increments passed character pointer beyond line,
 *                    fills return_buf with return_buf[0] = opcode, return_buf[1] = low byte of operand, return_buf[2] = high byte of operand, return_buf[3] = bytes in opcode + operand
 */
void parse_line(uint8_t **curr_char_dbl_ptr, uint32_t line_number, uint8_t *return_buf) {
    // determine opcode
    skip_whitespace(curr_char_dbl_ptr);
    uint8_t opcode = determine_opcode(curr_char_dbl_ptr);
    if (opcode == INVALID_OPCODE) {
        fprintf(stderr, "Invalid opcode at line %d\n", line_number);
        exit(ERR_INVALID_OPERAND_OPCODE);
    } else if (opcode == OP_SKIP) {
        return_buf[3] = 0;
        return;
    }
    
    // determine operand and addressing mode
    skip_whitespace(curr_char_dbl_ptr);
    memset(return_buf, INVALID_ADDR_MODE, 3); // set addressing mode to something invalid so we know if we have error
    determine_operand_and_addressing(curr_char_dbl_ptr, return_buf);
    if (return_buf[0] == INVALID_ADDR_MODE) {
        fprintf(stderr, "Invalid operand at line %d\n", line_number);
        exit(ERR_INVALID_OPERAND_OPCODE);
    }

    skip_whitespace(curr_char_dbl_ptr);

    // advance past comment if there is one
    if ((**curr_char_dbl_ptr == ';')) {
        next_line(curr_char_dbl_ptr);
    }
    // check for newline/end of program at end of line
    if ((**curr_char_dbl_ptr != '\n') && (**curr_char_dbl_ptr != 0)) {
        fprintf(stderr, "Invalid syntax at line %d\n", line_number);
        exit(ERR_SYNTAX);
    }

    if (return_buf[0] == ADDR_MODE_IMP ||
        return_buf[0] == ADDR_MODE_ACCUM_PARSE) {
        return_buf[3] = 1;
    } else if (return_buf[0] == ADDR_MODE_ABS ||
                return_buf[0] == ADDR_MODE_ABS_Y ||
                return_buf[0] == ADDR_MODE_ABS_X ||
                return_buf[0] == ADDR_MODE_IND) {
        return_buf[3] = 3;
    } else {
        return_buf[3] = 2;
    }

    return_buf[0] = (*ptr_arr[opcode])(opcode, return_buf[0]);
    if (return_buf[0] == 0xFF) {
        fprintf(stderr, "Invalid addressing mode at line %d\n", line_number);
        exit(ERR_INVALID_OPERAND_OPCODE);
    }

    // advance to next line if not at the end of the program
    if (**curr_char_dbl_ptr == '\n') {
        (*curr_char_dbl_ptr)++;
    }
}

/*
 */
 // search for .keywords (assembler directives), labels
 // Store load addresses with respective lines
 // Use initial label declarations to populate table, then manage label tokens after
 // probably need to tokenize...

/* assembly_to_bytecode
 *      DESCRIPTION: converts passed file to 6502 bytecode
 *      INPUTS: file_path -- file to convert to bytecode
 *      OUTPUTS: pointer to bytecode to run
 *      SIDE EFFECTS: none
 */
Program_t *clip_to_program(Clip_t *c) {
    Program_t *p = new_program(PROGRAM_INIT_SIZE);
    Table_t *label_table = new_table(TABLE_INIT_SIZE);

    uint8_t *opcode_operand_buf = (uint8_t *)malloc(4);

    for (int i = 0; i < c->index; i++) {
        // default roll at 0x8000 may be empty
        if (c->start[i].index == 0) {
            continue;
        }
        open_bytecode(p, c->start[i].start_address, c->start[i].start->line_num);
    }
    while (*curr_char_ptr != '\0') {
        parse_line(&curr_char_ptr, line_number, opcode_operand_buf);
        add_to_bytecode(bc, opcode_operand_buf, opcode_operand_buf[3], line_number);
        line_number++;
    }

    free(sf_asm);

    return bc;
}
