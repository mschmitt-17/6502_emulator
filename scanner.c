#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lib.h"
#include "scanner.h"
#include "6502.h"

#define CLIP_INIT_SIZE      256

/* skip_whitespace
 *      DESCRIPTION: helper function used to advance passed pointer beyond spaces and tabs
 *      INPUTS: curr_char_dbl_ptr -- double pointer to advance past spaces/tabs
 *      OUTPUTS: number of spaces skipped
 *      SIDE EFFECTS: increments passed pointer beyond spaces/tabs
 */
static uint32_t skip_whitespace(uint8_t **curr_char_dbl_ptr) {
    uint32_t spaces_skipped = 0;
    while ((**curr_char_dbl_ptr == ' ') || (**curr_char_dbl_ptr == '\t')) {
        spaces_skipped++;
        (*curr_char_dbl_ptr)++;
    }
    return spaces_skipped;
}

/* skip_until_whitespace
 *      DESCRIPTION: helper function used to advance passed pointer until space, tab, return, or newline
 *      INPUTS: curr_char_dbl_ptr -- double pointer to advance until whitespace
 *      OUTPUTS: number of characters skipped
 *      SIDE EFFECTS: increments passed pointer until whitespace
 */
static uint32_t skip_until_whitespace(uint8_t **curr_char_dbl_ptr) {
    uint32_t chars_skipped = 0;
    while ((**curr_char_dbl_ptr != ' ') && (**curr_char_dbl_ptr != '\t') && (**curr_char_dbl_ptr != '\r') && (**curr_char_dbl_ptr != '\n')) {
        chars_skipped++;
        (*curr_char_dbl_ptr)++;
    }
    return chars_skipped;
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

/* run_directive
 *      DESCRIPTION: determines and runs assembly directive, erroring out for invalid directive
 *      INPUTS: c -- current clip being populated (passed in since .ORG will close clip)
 *              sf_asm -- pointer to assembly being processed into tokens
 *              directive_token -- token corresponding to directive being run
 *              operand_token -- token corresponding to operand of directive being run
 *      OUTPUTS: number of bytes to increment PC after directive is run
 *      SIDE EFFECTS: various, depends on specific directive being run
 */
static uint8_t run_directive(Clip_t *c, uint8_t *sf_asm, Token_t directive_token, Token_t operand_token) {
    if (directive_token.end_index - directive_token.start_index + 1 == 4) {
        // directives only take absolute addressing
        if (operand_token.end_index - operand_token.start_index + 1 == 5 && *(sf_asm + operand_token.start_index) == '$') {
            if (*(sf_asm + directive_token.start_index + 1) == 'O' || *(sf_asm + directive_token.start_index + 1) == 'o') {
                if (*(sf_asm + directive_token.start_index + 2) == 'R' || *(sf_asm + directive_token.start_index + 2) == 'r') {
                    if (*(sf_asm + directive_token.start_index + 3) == 'G' || *(sf_asm + directive_token.start_index + 3) == 'g') {
                        // ORG: indicates where to load following lines of bytecode
                        uint16_t load_address = 0x0000;
                        
                        for (int i = 0; i < 4; i++) {
                            if (!is_hex_number(*(sf_asm + operand_token.start_index + i + 1))) {
                                fprintf(stderr, "Invalid operand at line %d\n", operand_token.line_num);
                                exit(ERR_INVALID_OPERAND_OPCODE);
                            }
                            load_address |= (char_to_hex(*(sf_asm + operand_token.start_index + i + 1))) << i * 4;
                        }
                        
                        close_roll(c->start + c->index - 1);
                        open_roll(c, load_address, directive_token.line_num);
                        return 0;
                    }
                }
            }
        }
    }
    fprintf(stderr, "Invalid directive at line %d\n", directive_token.line_num);
    exit(ERR_INVALID_DIRECTIVE);
}

/* operand_num_bytes
 *      DESCRIPTION: determines number of bytes of operand corresponding to passed arguments (used to increment PC)
 *      INPUTS: curr_char_ptr -- pointer to beginning character of operand
 *              token_len -- number of characters in operand
 *      OUTPUTS: number of bytes for current operand or -1 if invalid operand
 *      SIDE EFFECTS: none
 */
static uint8_t operand_num_bytes(uint8_t *curr_char_ptr, uint8_t token_len) {
    switch (token_len) {
        case 1:
            // accumulator
            return 0x00;
        case 3:
        case 4:
            // zpg/immediate
            return 0x01;
        case 5:
            if (is_hex_number(*(curr_char_ptr + 3))) {
                // assume absolute addressing mode
                return 0x02;
            } else {
                // assume 0-page x-index or 0-page y-index
                return 0x01;
            }
        case 7:
            if (*curr_char_ptr == '(') {
                if (is_hex_number(*(curr_char_ptr + 4))) {
                    // assume indirect addressing mode
                    return 0x02;
                } else {
                    // assume indirect x/y-indexed addressing mode
                    return 0x01;
                }
            } else {
                //assume absolute x/y-indexed addressing mode
                return 0x02;
            }
        default:
            return 0xFF;
    }
}

/* check_instruction
 *      DESCRIPTION: determines if instruction at passed pointer is valid or not
 *      INPUTS: curr_char_ptr -- pointer to beginning character of instruction
 *      OUTPUTS: 1 if instruction is valid, 0 otherwise
 *      SIDE EFFECTS: none
 */
static uint8_t check_instruction(uint8_t *curr_char_ptr) {
    switch (*curr_char_ptr) {
        case 'A':
            switch (*(++curr_char_ptr)) {
                case 'D':
                    if (*(++curr_char_ptr) == 'C') {
                        return 1;
                    }
                    break;
                case 'N':
                    if (*(++curr_char_ptr) == 'D') {
                        return 1;
                    }
                    break;
                case 'S':
                    if (*(++curr_char_ptr) == 'L') {
                        return 1;
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
                    if (*curr_char_ptr == 'C' || *curr_char_ptr == 'S') {
                        return 1;
                    }
                    break;
                case 'E':
                    if (*(++curr_char_ptr) == 'Q') {
                        return 1;
                    }
                    break;
                case 'I':
                    if (*(++curr_char_ptr) == 'T') {
                        return 1;
                    }
                    break;
                case 'M':
                    if (*(++curr_char_ptr) == 'I') {
                        return 1;
                    }
                    break;
                case 'N':
                    if (*(++curr_char_ptr) == 'E') {
                        return 1;
                    }
                    break;
                case 'P':
                    if (*(++curr_char_ptr) == 'L') {
                        return 1;
                    }
                    break;
                case 'R':
                    if (*(++curr_char_ptr) == 'K') {
                        return 1;
                    }
                    break;
                case 'V':
                    curr_char_ptr++;
                    if (*curr_char_ptr == 'C' || *curr_char_ptr == 'S') {
                        return 1;
                    }
                    break;
                default:
                    break;
            }
            break;
        
        case 'C':
            switch (*(++curr_char_ptr)) {
                case 'L':
                    curr_char_ptr++;
                    if (*curr_char_ptr == 'C' || *curr_char_ptr == 'D' || *curr_char_ptr == 'I' || *curr_char_ptr == 'V') {
                        return 1;
                    }
                    break;
                case 'M':
                    if (*(++curr_char_ptr) == 'P') {
                        return 1;
                    }
                    break;
                case 'P':
                    curr_char_ptr++;
                    if (*curr_char_ptr == 'X' || *curr_char_ptr == 'Y') {
                        return 1;
                    }
                    break;
                default:
                    break;
            }
            break;
        
        case 'D':
            if (*(++curr_char_ptr) == 'E') {
                curr_char_ptr++;
                if (*curr_char_ptr == 'C' || *curr_char_ptr == 'X' || *curr_char_ptr == 'Y') {
                    return 1;
                }
            }
            break;
        
        case 'E':
            if (*(++curr_char_ptr) == 'O') {
                if (*(++curr_char_ptr) == 'R') {
                    return 1;
                }
            }
            break;
        
        case 'I':
            if (*(++curr_char_ptr) == 'N') {
                curr_char_ptr++;
                if (*curr_char_ptr == 'C' || *curr_char_ptr == 'X' || *curr_char_ptr == 'Y') {
                    return 1;
                }
            }
            break;
        
        case 'J':
            curr_char_ptr++;
            if (*curr_char_ptr == 'M') {
                if (*(++curr_char_ptr) == 'P') {
                    return 1;
                }
            } else if (*curr_char_ptr == 'S') {
                if (*(++curr_char_ptr) == 'R') {
                    return 1;
                }
            }
            break;
        
        case 'L':
            curr_char_ptr++;
            if (*curr_char_ptr == 'D') {
                curr_char_ptr++;
                if (*curr_char_ptr == 'A' || *curr_char_ptr == 'X' || *curr_char_ptr == 'Y') {
                    return 1;
                }
            } else if (*curr_char_ptr == 'S') {
                if (*(++curr_char_ptr) == 'R') {
                    return 1;
                }
            }
            break;

        case 'N':
            if (*(++curr_char_ptr) == 'O') {
                if (*(++curr_char_ptr) == 'P') {
                    return 1;
                }
            }
            break;
        
        case 'O':
            if (*(++curr_char_ptr) == 'R') {
                if (*(++curr_char_ptr) == 'A') {
                    return 1;
                }
            }
            break;
        
        case 'P':
            curr_char_ptr++;
            if (*curr_char_ptr == 'H') {
                curr_char_ptr++;
                if (*curr_char_ptr == 'A' || *curr_char_ptr == 'P') {
                    return 1;
                }
            } else if (*curr_char_ptr == 'L') {
                curr_char_ptr++;
                if (*curr_char_ptr == 'A' || *curr_char_ptr == 'P') {
                    return 1;
                }
            }
            break;
        
        case 'R':
            curr_char_ptr++;
            if (*curr_char_ptr == 'O') {
                curr_char_ptr++;
                if (*curr_char_ptr == 'L' || *curr_char_ptr == 'R') {
                    return 1;
                }
            } else if (*curr_char_ptr == 'T') {
                curr_char_ptr++;
                if (*curr_char_ptr == 'I' || *curr_char_ptr == 'S') {
                    return 1;
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
                    curr_char_ptr++;
                    if (*curr_char_ptr == 'C' || *curr_char_ptr == 'D' || *curr_char_ptr == 'I') {
                        return 1;
                    }
                    break;
                case 'T':
                    curr_char_ptr++;
                    if (*curr_char_ptr == 'A' || *curr_char_ptr == 'X' || *curr_char_ptr == 'Y') {
                        return 1;
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
                    if (*curr_char_ptr == 'X' || *curr_char_ptr == 'Y') {
                        return 1;
                    }
                    break;
                case 'S':
                    if (*(++curr_char_ptr) == 'X') {
                        return 1;
                    }
                    break;
                case 'X':
                    curr_char_ptr++;
                    if (*curr_char_ptr == 'A' || *curr_char_ptr == 'S') {
                        return OP_TXA;
                    }
                    break;
                case 'Y':
                    if (*(++curr_char_ptr) == 'A') {
                        return 1;
                    }
                    break;
                default:
                    break;
            }
            break;
    
        default:
            break;
    }

    return 0;
}

/* validate_label
 *      DESCRIPTION: since labels may only contain select characters (source: ChatGPT), this helper validates all characters in a label
 *      INPUTS: curr_char_ptr -- pointer to beginning character of label
 *              token_len -- number of characters in label
 *      OUTPUTS: 1 if label is valid, 0 if label is invalid
 *      SIDE EFFECTS: none
 */
static uint8_t validate_label(uint8_t *curr_char_ptr, uint8_t token_len) {
    int i = 0;
    
    if (token_len >= 1) {
        if (!is_letter(curr_char_ptr[i]) && curr_char_ptr[i] != '_') {
            return 0;
        }
        i++;
    }
    
    for (i; i < token_len; i++) {
        if (!is_letter(curr_char_ptr[i]) && !is_decimal_number(curr_char_ptr[i]) && curr_char_ptr[i] != '_') {
            return 0;
        }
    }
    return 1;
}

/* validate_and_add_label
 *      DESCRIPTION: validates and adds label to table if label is valid
 *      INPUTS: sf_asm -- pointer to assembly being tokenized
 *              label_token -- token of label
 *              label_table_dbl_ptr -- double pointer to label table
 *              scan_pc -- pointer to pc used in scanning   
 *      OUTPUTS: 1 if label was validated and added, 0 otherwise
 *      SIDE EFFECTS: adds label to table if valid
 */
static uint8_t validate_and_add_label(uint8_t *sf_asm, Token_t label_token, Table_t **label_table_dbl_ptr, uint16_t *scan_pc) {
    if (validate_label(sf_asm + label_token.start_index, label_token.end_index - label_token.start_index + 1)) {
        char label_str[label_token.end_index - label_token.start_index + 2];
        memset(label_str, '\0', label_token.end_index - label_token.start_index + 2);
        memcpy(label_str, sf_asm + label_token.start_index, label_token.end_index - label_token.start_index + 1);
        add_to_table(label_table_dbl_ptr, label_str, *scan_pc);
        return 1;
    } else {
        return 0;
    }
}

/* scan_line
 *      DESCRIPTION: scans line into tokens and adds tokens to clip, running assembler directives and adding labels to label table on the way
 *      INPUTS: c -- pointer to clip containing rolls where tokens are to be added
 *              sf_asm -- pointer to assembly being tokenized
 *              curr_char_dbl_ptr -- double pointer to current character in sf_asm being processed
 *              line_number -- number of line being scanned
 *              scan_pc -- pointer to pc tracking scanned lines
 *              label_table_dbl_ptr -- double pointer to label table
 *      OUTPUTS: none
 *      SIDE EFFECTS: adds tokens to clip, advances curr_char_dbl_ptr to next line, advances scan_pc, runs assembler directives, adds labels to lable table
 */
static void scan_line(Clip_t *c, uint8_t* sf_asm, uint8_t **curr_char_dbl_ptr, uint32_t line_number, uint16_t *scan_pc, Table_t **label_table_dbl_ptr) {

    uint8_t directive_run = 0;

    skip_whitespace(curr_char_dbl_ptr);
    if (**curr_char_dbl_ptr == ';') {
        // whole line is a comment
        next_line(curr_char_dbl_ptr);
    } else {
        // determine number of, start/end index of tokens
        uint8_t num_tokens = 0;
        Token_t token_buf[3];
        while ((**curr_char_dbl_ptr != '\n' && **curr_char_dbl_ptr != '\0' && **curr_char_dbl_ptr != ';')) {
            uint32_t token_start_index = *curr_char_dbl_ptr - sf_asm;
            uint32_t token_end_index = *curr_char_dbl_ptr - sf_asm + skip_until_whitespace(curr_char_dbl_ptr) - 1;
            token_buf[num_tokens].start_index = token_start_index;
            token_buf[num_tokens].end_index = token_end_index;
            token_buf[num_tokens].line_num = line_number;
            num_tokens++;
            skip_whitespace(curr_char_dbl_ptr);
        }

        uint8_t curr_token = 0;
        if (num_tokens) {
            if (num_tokens == 1) {
                // if there's just one token, assume it has to be an instruction
                token_buf[curr_token++].type = TOKEN_INSTRUCTION;
                (*scan_pc)++;
            } else {
                // if we have 3 tokens, we know the first one has to be a label
                if (num_tokens == 3) {
                    if (validate_and_add_label(sf_asm, token_buf[curr_token], label_table_dbl_ptr, scan_pc)) {
                        token_buf[curr_token++].type = TOKEN_LABEL;
                    } else {
                        fprintf(stderr, "Error at line %d: invalid label syntax\n", token_buf[curr_token].line_num);
                        exit(ERR_SYNTAX);
                    }
                }

                // if final token is an instruction, then we need to have LABEL INSTRUCTION
                if ((token_buf[curr_token + 1].end_index - token_buf[curr_token + 1].start_index + 1) == 3 &&
                        check_instruction(sf_asm[token_buf[curr_token + 1].start_index])) {
                    if (validate_and_add_label(sf_asm, token_buf[curr_token], label_table_dbl_ptr, scan_pc)) {
                        token_buf[curr_token++].type = TOKEN_LABEL;
                    } else {
                        fprintf(stderr, "Error at line %d: invalid label syntax\n", token_buf[curr_token].line_num);
                        exit(ERR_SYNTAX);
                    }
                    token_buf[curr_token++].type = TOKEN_INSTRUCTION;
                    scan_pc++;
                } 
                
                // if the above is not true, we may have ASSEMBLER-DIRECTIVE OPERAND or INSTRUCTION OPERAND/LABEL
                else {
                    if (sf_asm[token_buf[curr_token].start_index] == '.') {
                        *scan_pc = (*scan_pc) + run_directive(c, sf_asm, token_buf[curr_token], token_buf[curr_token + 1]);
                        directive_run = 1;
                    } else {
                        token_buf[curr_token++].type = TOKEN_INSTRUCTION;
                        (*scan_pc)++;
                        // if length 1 and not A (for accumulator) or length > 1 but starting character is not letter or underscore, assume token is a label
                        if (((token_buf[curr_token].end_index == token_buf[curr_token].start_index) &&
                              sf_asm[token_buf[curr_token].start_index] != 'A' &&
                              sf_asm[token_buf[curr_token].start_index] != 'a') ||
                            ((token_buf[curr_token].end_index - token_buf[curr_token].start_index + 1 > 1) &&
                            (is_letter(sf_asm[token_buf[curr_token].start_index]) || sf_asm[token_buf[curr_token].start_index] == '_'))) {
                            token_buf[curr_token++].type = TOKEN_LABEL;
                            (*scan_pc)++; // labels generate 1-byte relative offsets in bytecode    
                        } else {
                            uint8_t operand_bytes = operand_num_bytes(sf_asm + token_buf[curr_token].start_index, token_buf[curr_token].end_index - token_buf[curr_token].start_index + 1);
                            if (operand_bytes == 0xFF) {
                                fprintf(stderr, "Error at line %d: invalid operand\n", token_buf[curr_token].line_num);
                                exit(ERR_SYNTAX);
                            }
                            *scan_pc = (*scan_pc) + operand_bytes;
                            token_buf[curr_token++].type = TOKEN_OPERAND;
                        }
                    }
                }
            } 
        }

        if (!directive_run) {
            int i = 0;
            // skip any label declaration tokens
            if (token_buf[0].type == TOKEN_LABEL) {
                i++;
            }
            for (i; i < curr_token; i++) {
                add_to_roll(c->start + c->index - 1 , token_buf + i);
            }
        }

        // check for comment
        if (**curr_char_dbl_ptr == ';') {
            next_line(curr_char_dbl_ptr);
        }
    }
    
    // check for newline/end of program at end of line
    if ((**curr_char_dbl_ptr != '\n') && (**curr_char_dbl_ptr != '\0')) {
        fprintf(stderr, "Invalid syntax at line %d\n", line_number);
        exit(ERR_SYNTAX);
    }

    // advance to next line if not at the end of the program
    if (**curr_char_dbl_ptr == '\n') {
        (*curr_char_dbl_ptr)++;
    }
} 

/* assembly_to_clip
 *      DESCRIPTION: scans assembly into clip, filling out label table as well
 *      INPUTS: sf_asm -- pointer to assembly being processed
 *              label_table_dbl_ptr -- double pointer to label table to be populated
 *      OUTPUTS: pointer to clip containing tokens scanned from assembly
 *      SIDE EFFECTS: populates passed label table
 */
Clip_t *assembly_to_clip(uint8_t* sf_asm, Table_t **label_table_dbl_ptr) {
    uint8_t *curr_char_ptr = sf_asm;
    uint32_t line_number = 1;
    uint16_t scan_pc = 0x8000;
    Clip_t *c = new_clip(CLIP_INIT_SIZE);

    while (*curr_char_ptr != '\0') {
        scan_line(c, sf_asm, &curr_char_ptr, line_number, &scan_pc, label_table_dbl_ptr);
        line_number++;
    }

    close_roll(c->start + c->index - 1); // close off final roll

    return c;
}
