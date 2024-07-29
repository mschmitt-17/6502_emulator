#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib.h"
#include "generator.h"
#include "6502.h"
#include "addressing_jumptable.h"
#include "table.h"

#define PROGRAM_INIT_SIZE       256

/* determine_opcode
 *      DESCRIPTION: determines opcode of token corresponding to passed pointer
 *      INPUTS: curr_char_ptr -- pointer to starting character of instruction token
 *      OUTPUTS: opcode of line on success, 0xFF on failure
 *      SIDE EFFECTS: none
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
 *      INPUTS: curr_char_ptr -- pointer to starting character of operand token
 *              token_len -- length of operand token
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
                        return_buf[1] = (char_to_hex(*(curr_char_ptr + 1)) << 4) | char_to_hex(*(curr_char_ptr + 2));
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
                            return_buf[1] = (char_to_hex(*(curr_char_ptr + 2)) << 4) | char_to_hex(*(curr_char_ptr + 3));
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
                                return_buf[1] = (char_to_hex(*(curr_char_ptr + 1)) << 4) | char_to_hex(*(curr_char_ptr + 2));
                            } else if (*(curr_char_ptr + 4) == 'Y' || *(curr_char_ptr + 4) == 'y') {
                                // OPC $LL,Y -- zero-page y-indexed addressing mode
                                return_buf[0] = ADDR_MODE_ZPG_Y;
                                return_buf[1] = (char_to_hex(*(curr_char_ptr + 1)) << 4) | char_to_hex(*(curr_char_ptr + 2));
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
                                        return_buf[1] = (char_to_hex(*(curr_char_ptr + 4)) << 4) | char_to_hex(*(curr_char_ptr + 5));
                                        return_buf[2] = (char_to_hex(*(curr_char_ptr + 2)) << 4) | char_to_hex(*(curr_char_ptr + 3));
                                    }
                                }
                            } else if (*(curr_char_ptr + 4) == ',') {
                                if (*(curr_char_ptr + 5) == 'X') {
                                    if (*(curr_char_ptr + 6) == ')') {
                                        // OPC ($LL,X) -- indirect x-indexed addressing mode
                                        return_buf[0] = ADDR_MODE_IND_X;
                                        return_buf[1] = (char_to_hex(*(curr_char_ptr + 2)) << 4) | char_to_hex(*(curr_char_ptr + 3));
                                    }
                                }
                            } else if (*(curr_char_ptr + 4) == ')') {
                                if (*(curr_char_ptr + 5) == ',') {
                                    if (*(curr_char_ptr + 6) == 'Y') {
                                        // OPC ($LL),Y -- indirect y-indexed addressing mode
                                        return_buf[0] = ADDR_MODE_IND_Y;
                                        return_buf[1] = (char_to_hex(*(curr_char_ptr + 2)) << 4) | char_to_hex(*(curr_char_ptr + 3));
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
                                        return_buf[1] = (char_to_hex(*(curr_char_ptr + 3)) << 4) | char_to_hex(*(curr_char_ptr + 4));
                                        return_buf[2] = (char_to_hex(*(curr_char_ptr + 1)) << 4) | char_to_hex(*(curr_char_ptr + 2));
                                    } else if (*(curr_char_ptr + 6) == 'Y' || *(curr_char_ptr + 6) == 'y') {
                                        // OPC $HHLL,Y -- absolute x-indexed addressing mode
                                        return_buf[0] = ADDR_MODE_ABS_Y;
                                        return_buf[1] = (char_to_hex(*(curr_char_ptr + 3)) << 4) | char_to_hex(*(curr_char_ptr + 4));
                                        return_buf[2] = (char_to_hex(*(curr_char_ptr + 1)) << 4) | char_to_hex(*(curr_char_ptr + 2));
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
}

/* generate_line
 *      DESCRIPTION: determines opcode and operand for current line and adds to passed return buffer 
 *      INPUTS: bc -- pointer to bytecode that line is to be added to
 *              sf_asm -- pointer to assembly which bytecode is being generated from
 *              curr_token_dbl_ptr -- double pointer to token which code is being generated from
 *              return_buf -- buffer to write opcode and operand to
 *              label_table_dbl_ptr -- double pointer to table which contains all labels for the current program
 *      OUTPUTS: none
 *      SIDE EFFECTS: increments passed token pointer beyond line,
 *                    fills return_buf with return_buf[0] = opcode, return_buf[1] = low byte of operand, return_buf[2] = high byte of operand, return_buf[3] = bytes in opcode + operand
 */
static void generate_line(Bytecode_t *bc, uint8_t *sf_asm, Token_t **curr_token_dbl_ptr, uint8_t *return_buf, Table_t **label_table_dbl_ptr) {
    uint32_t curr_line = (*curr_token_dbl_ptr)->line_num;
    uint8_t opcode = INVALID_OPCODE;
    memset(return_buf, ADDR_MODE_IMP, 4);

    while ((*curr_token_dbl_ptr)->type != TOKEN_END && (*curr_token_dbl_ptr)->line_num == curr_line) {
        switch ((*curr_token_dbl_ptr)->type) {
            case TOKEN_LABEL:
                char *label_str = (char *)malloc((*curr_token_dbl_ptr)->end_index - (*curr_token_dbl_ptr)->start_index + 2);
                memset(label_str, '\0', (*curr_token_dbl_ptr)->end_index - (*curr_token_dbl_ptr)->start_index + 2);
                memcpy(label_str, sf_asm + (*curr_token_dbl_ptr)->start_index, (*curr_token_dbl_ptr)->end_index - (*curr_token_dbl_ptr)->start_index + 1);
                    if (return_buf[0] != ADDR_MODE_IMP) {
                        fprintf(stderr, "Syntax error at line %d: can't have operand and label referenced on same line\n", (*curr_token_dbl_ptr)->line_num);
                        exit(ERR_SYNTAX);
                    }
                    return_buf[0] = ADDR_MODE_ABS; // relative addressing is syntactically the same as absolute addressing

                    // if offset will overflow beyond -128 or 127, error out
                    if (((get_value(*label_table_dbl_ptr, label_str) > (bc->load_address + bc->index + 1)) &&
                         (get_value(*label_table_dbl_ptr, label_str) - (bc->load_address + bc->index + 1)) > 0x7F) || 
                        ((get_value(*label_table_dbl_ptr, label_str) < (bc->load_address + bc->index + 1)) &&
                         ((bc->load_address + bc->index + 1) - get_value(*label_table_dbl_ptr, label_str)) > 0x80)) {
                        fprintf(stderr, "Error at line %d: branch offsets may be at most -128 or 127 bytes away\n", (*curr_token_dbl_ptr)->line_num);
                        exit(ERR_OVERFLOW);
                    }

                    return_buf[1] = get_value(*label_table_dbl_ptr, label_str) - (bc->load_address + bc->index + 2); // bc->start + bc->index + 2 = previously filled entries plus where pc will be after running INST LABEL line
                free(label_str);
                break;
            case TOKEN_INSTRUCTION:
                if (opcode != INVALID_OPCODE) {
                    fprintf(stderr, "Syntax error at line %d: can't have two instructions in one line\n", (*curr_token_dbl_ptr)->line_num);
                    exit(ERR_SYNTAX);
                }
                opcode = determine_opcode(sf_asm + (*curr_token_dbl_ptr)->start_index);
                if (opcode == INVALID_OPCODE) {
                    fprintf(stderr, "Invalid opcode at line %d\n", (*curr_token_dbl_ptr)->line_num);
                    exit(ERR_INVALID_OPERAND_OPCODE);
                }
                break;
            case TOKEN_OPERAND:
                if (return_buf[0] != ADDR_MODE_IMP) {
                    fprintf(stderr, "Syntax error at line %d: can't have two operands in one line\n", (*curr_token_dbl_ptr)->line_num);
                    exit(ERR_SYNTAX);
                }
                determine_operand_and_addressing(sf_asm + (*curr_token_dbl_ptr)->start_index, (*curr_token_dbl_ptr)->end_index - (*curr_token_dbl_ptr)->start_index + 1, return_buf);
                if (return_buf[0] == ADDR_MODE_IMP) {
                    fprintf(stderr, "Invalid operand at line %d\n", (*curr_token_dbl_ptr)->line_num);
                    exit(ERR_INVALID_OPERAND_OPCODE);
                }
                break;
            default:
                break;
        }
        (*curr_token_dbl_ptr)++;
    }

    if (return_buf[0] == ADDR_MODE_IMP ||
        return_buf[0] == ADDR_MODE_ACCUM_GEN) {
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
        fprintf(stderr, "Invalid addressing mode at line %d\n", curr_line);
        exit(ERR_INVALID_ADDRESSING_MODE);
    }
}

/* roll_to_bytecode
 *      DESCRIPTION: converts all of passed roll into bytecode
 *      INPUTS: bc -- pointer to bytecode that is to be populated
 *              r -- pointer to roll from which bytecode is to be generated
 *              sf_asm -- pointer to assembly which tokens index into
 *              label_table_dbl_ptr -- double pointer to table which labels in program use
 *      OUTPUTS: none
 *      SIDE EFFECTS: populates passed bytecode with generated code corresponding to tokens in roll
 */
static void roll_to_bytecode(Bytecode_t *bc, Roll_t *r, uint8_t *sf_asm, Table_t **label_table_dbl_ptr) {
    Token_t *curr_token = r->start;
    uint8_t opcode_operand_buf[4];
    while (curr_token->type != TOKEN_END) {
        generate_line(bc, sf_asm, &curr_token, opcode_operand_buf, label_table_dbl_ptr);
        add_to_bytecode(bc, opcode_operand_buf, opcode_operand_buf[3], curr_token->line_num - 1);
    }
}

/* clip_to_program
 *      DESCRIPTION: converts passed clip to program
 *      INPUTS: sf_asm -- array of assembly code which clip tokens index into
 *              c -- clip to convert into program
 *              label_table_dbl_ptr -- double pointer to label table populated during scanning
 *      OUTPUTS: pointer to program to run
 *      SIDE EFFECTS: none
 */
Program_t *clip_to_program(uint8_t *sf_asm, Clip_t *c, Table_t **label_table_dbl_ptr) {
    Program_t *p = new_program(PROGRAM_INIT_SIZE);

    uint8_t *opcode_operand_buf = (uint8_t *)malloc(4);

    for (int i = 0; i < c->index; i++) {
        // default roll at 0x8000 may be empty
        if (c->start[i].index == 0) {
            continue;
        }
        open_bytecode(p, c->start[i].start_address, c->start[i].start->line_num);
        roll_to_bytecode(p->start + p->index - 1, c->start + c->index - 1, sf_asm, label_table_dbl_ptr);
    }

    return p;
}
