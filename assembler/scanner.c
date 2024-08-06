#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../lib/lib.h"
#include "scanner.h"

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

/* compare_characters
 *      DESCRIPTION: performs a character-by-character comparison of passed strings (used to verify assembly directive strings)
 *      INPUTS: cmpstr -- string to compare to reference string (i.e. string from assembly)
 *              refstr -- reference string for directive
 *              num_chars -- characters to compare
 *      OUTPUTS: 0 if strings do not match character-for-character, 1 if strings match character-for-character
 *      SIDE EFFECTS: none
 */
static uint32_t compare_characters(char *cmpstr, char *refstr, uint32_t num_chars) {
    for (int i = 0; i < num_chars; i++) {
        if (cmpstr[i] != refstr[i]) {
            return 0;
        }
    }
    return 1;
}

/* run_directive
 *      DESCRIPTION: determines and runs assembly directive, erroring out for invalid directive
 *      INPUTS: sf -- 6502 on which directives are to be run
 *              c -- current clip being populated (passed in since .ORG will close clip)
 *              sf_asm -- pointer to assembly being processed into tokens
 *              curr_char_dbl_ptr -- double pointer to current character in assembly being processed (needed for .END directive)
 *              directive_token -- token corresponding to directive being run
 *              operand_token -- token corresponding to operand of directive being run
 *      OUTPUTS: none
 *      SIDE EFFECTS: various, depends on specific directive being run
 */
static void run_directive(sf_t *sf, Clip_t *c, uint8_t *sf_asm, uint8_t **curr_char_dbl_ptr, Token_t directive_token, Token_t operand_token) {
    if (directive_token.end_index - directive_token.start_index + 1 == 4) {
        if (operand_token.type != TOKEN_EMPTY) {
            // directives that take absolute addressing
            if (operand_token.end_index - operand_token.start_index + 1 == 5 && *(sf_asm + operand_token.start_index) == '$') {
                if (compare_characters(sf_asm + directive_token.start_index + 1, "ORG", 3) || compare_characters(sf_asm + directive_token.start_index + 1, "org", 3)) {
                    // ORG: indicates where to load following lines of bytecode
                    uint16_t load_address = 0x0000;
                    
                    for (int i = 0; i < 4; i++) {
                        if (!is_hex_number(*(sf_asm + operand_token.start_index + i + 1))) {
                            fprintf(stderr, "Invalid operand at line %d\n", operand_token.line_num);
                            exit(ERR_INVALID_OPERAND_OPCODE);
                        }
                        load_address |= (char_to_hex(*(sf_asm + operand_token.start_index + i + 1))) << ((3 - i) * 4);
                    }
                    
                    close_roll(c->start + c->index - 1);
                    open_roll(c, load_address, directive_token.line_num);
                    sf->pc = load_address;
                    return;
                }
            }               
        } else {
            if (compare_characters(sf_asm + directive_token.start_index + 1, "END", 3) || compare_characters(sf_asm + directive_token.start_index + 1, "end", 3)) {
                // END: ignore all subsequent code
                while (**curr_char_dbl_ptr != '\0') {
                    next_line(curr_char_dbl_ptr);
                    if (**curr_char_dbl_ptr == '\n') {
                        (*curr_char_dbl_ptr)++;
                    }
                }
                return;
            }
        }
    } else if (directive_token.end_index - directive_token.start_index + 1 == 5) {
        // absolute addressing
        if (operand_token.end_index - operand_token.start_index + 1 == 5 && *(sf_asm + operand_token.start_index) == '$') {
            if (compare_characters(sf_asm + directive_token.start_index + 1, "WORD", 4) || compare_characters(sf_asm + directive_token.start_index + 1, "word", 4)) {
                // WORD: loads word contained in operand at PC
                uint16_t load_address = 0x0000;
                
                for (int i = 0; i < 4; i++) {
                    if (!is_hex_number(*(sf_asm + operand_token.start_index + i + 1))) {
                        fprintf(stderr, "Invalid operand at line %d\n", operand_token.line_num);
                        exit(ERR_INVALID_OPERAND_OPCODE);
                    }
                    load_address |= (char_to_hex(*(sf_asm + operand_token.start_index + i + 1))) << ((3 - i) * 4);
                }

                // word is stored high byte low byte
                sf->memory[sf->pc++] = load_address & 0x00FF;
                sf->memory[sf->pc++] = (load_address & 0xFF00) >> 8;
                
                return;
            }
        }
    }
    fprintf(stderr, "Invalid directive at line %d\n", directive_token.line_num);
    exit(ERR_INVALID_DIRECTIVE);
}

/* operand_num_bytes
 *      DESCRIPTION: determines number of bytes passed operand will take depending on addressing mode
 *      INPUTS: sf_asm --  pointer to assembly
 *              operand_token -- token corresponding to operand whose bytes we which to determine
 *      OUTPUTS: number of bytes operand will take up, or 3 if number of bytes will depend on preceding operand
 *      SIDE EFFECTS: none
 */
static uint8_t operand_num_bytes(uint8_t *sf_asm, Token_t operand_token) {
    char *operand_string = calloc(operand_token.end_index - operand_token.start_index + 2, 1);
    memcpy(operand_string, sf_asm + operand_token.start_index, operand_token.end_index - operand_token.start_index + 1);
    if (strlen(operand_string) == 1 && (operand_string[0] == 'A' || operand_string[0] == 'a')) {
        // OPC A -- Accumulator Addressing Mode
        return 0;
    } else if (operand_string[0] == '#') {
        // OPC #$BB -- Immediate Addressing Mode
        return 1;
    } else if (operand_string[0] == '$') {
        uint32_t chars_until_comma = (uint32_t)(strchr(operand_string, ',') - operand_string);
        if (chars_until_comma == 5) {
            // OPC $LLHH,X/Y -- Absolute X/Y indexed
            return 2;
        } else if (chars_until_comma == 3) {
            // OPC $LL,X/Y -- ZPG X/Y indexed
            return 1;
        } else {
            // OPC $LLHH -- Absolute
            return 2;
        }
    } else if (operand_string[0] == '(') {
        if (strchr(operand_string, ',')) {
            // OPC ($LL,X)/($LL),Y -- indirect X/Y indexed
            return 1;
        } else {
            // OPC ($LLHH) -- indirect
            return 2;
        }
    } else {
        // OPC LABEL -- may be absolute, absolute x-indexed, or absolute y-indexed
        // if instruction preceding this is branch, it must be relative
        return 3;
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

/* check_branch_instruction
 *      DESCRIPTION: determines if instruction at passed pointer is branch instruction or not
 *      INPUTS: curr_char_ptr -- pointer to beginning character of instruction
 *      OUTPUTS: 1 if instruction is branch instruction, 0 otherwise
 *      SIDE EFFECTS: none
 */
static uint8_t check_branch_instruction(uint8_t *curr_char_ptr) {
    if (*curr_char_ptr == 'B') {
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
            case 'V':
                curr_char_ptr++;
                if (*curr_char_ptr == 'C' || *curr_char_ptr == 'S') {
                    return 1;
                }
                break;
            default:
                break;
        }
    }
    return 0;
}

/* scan_line
 *      DESCRIPTION: scans line into tokens and adds tokens to clip, running assembler directives and adding labels to label table on the way
 *      INPUTS: sf -- pointer to 6502 which contains current PC
 *              c -- pointer to clip containing rolls where tokens are to be added
 *              sf_asm -- pointer to assembly being tokenized
 *              curr_char_dbl_ptr -- double pointer to current character in sf_asm being processed
 *              line_number -- number of line being scanned
 *              label_table_dbl_ptr -- double pointer to label table
 *      OUTPUTS: none
 *      SIDE EFFECTS: adds tokens to clip, advances curr_char_dbl_ptr to next line, advances scan_pc, runs assembler directives, adds labels to lable table
 */
static void scan_line(sf_t *sf, Clip_t *c, uint8_t* sf_asm, uint8_t **curr_char_dbl_ptr, uint32_t line_number, Table_t **label_table_dbl_ptr) {

    uint8_t directive_run = 0;

    skip_whitespace(curr_char_dbl_ptr);
    if (**curr_char_dbl_ptr == ';') {
        // whole line is a comment
        next_line(curr_char_dbl_ptr);
    } else if (**curr_char_dbl_ptr != '\n' && **curr_char_dbl_ptr != '\0') {
        // determine number of, start/end index of tokens
        uint8_t num_tokens = 0;
        Token_t token_buf[3];
        memset(token_buf, '\0', 3 * sizeof(Token_t));
        while ((**curr_char_dbl_ptr != '\n' && **curr_char_dbl_ptr != '\0' && **curr_char_dbl_ptr != ';')) {
            uint32_t token_start_index = *curr_char_dbl_ptr - sf_asm;
            uint32_t token_end_index = *curr_char_dbl_ptr - sf_asm + skip_until_whitespace(curr_char_dbl_ptr) - 1;
            token_buf[num_tokens].start_index = token_start_index;
            token_buf[num_tokens].end_index = token_end_index;
            token_buf[num_tokens].line_num = line_number;
            token_buf[num_tokens].type = TOKEN_PENDING;
            num_tokens++;
            skip_whitespace(curr_char_dbl_ptr);
        }

        for (int i = 0; i < num_tokens; i++) {
            if (sf_asm[token_buf[i].start_index] == '.') {
                // next token in buffer must either be operand or empty, so pass in token_buf[i + 1] as operand token
                run_directive(sf, c, sf_asm, curr_char_dbl_ptr, token_buf[i], token_buf[i + 1]);
                directive_run = 1;
            } else if ((token_buf[i].end_index - token_buf[i].start_index + 1 == 3) && check_instruction(sf_asm + token_buf[i].start_index)) {
                // instruction
                token_buf[i].type = TOKEN_INSTRUCTION;
                sf->pc++;
            } else if ((i == 0) && // check for label declaration
                        // valid if length 1 and NOT A/a (accumulator addressing) OR if length > 1 and starts with letter/underscore (we checked for instruction in above branch)
                        (((token_buf[i].end_index == token_buf[i].start_index) && 
                        sf_asm[token_buf[i].start_index] != 'A' &&
                        sf_asm[token_buf[i].start_index] != 'a') ||
                        ((token_buf[i].end_index - token_buf[i].start_index + 1 > 1) &&
                        (is_letter(sf_asm[token_buf[i].start_index]) || sf_asm[token_buf[i].start_index] == '_')))) {
                
                if (validate_label(sf_asm + token_buf[i].start_index, token_buf[i].end_index - token_buf[i].start_index + 1)) {
                    char label_str[token_buf[i].end_index - token_buf[i].start_index + 2];
                    memset(label_str, '\0', token_buf[i].end_index - token_buf[i].start_index + 2);
                    memcpy(label_str, sf_asm + token_buf[i].start_index, token_buf[i].end_index - token_buf[i].start_index + 1);
                    add_to_table(label_table_dbl_ptr, label_str, sf->pc);
                    token_buf[i].type = TOKEN_LABEL;
                } else {
                    fprintf(stderr, "Error at line %d: invalid label syntax\n", token_buf[i].line_num);
                    exit(ERR_SYNTAX);
                }
            
            } else {
                // operand
                if (i == 0) {
                    fprintf(stderr, "Error at line %d: operand referenced before instruction/directive\n", token_buf[i].line_num);
                    exit(ERR_SYNTAX);
                }

                // if directive has been run we've already consumed operand and incremented PC accordingly
                if (!directive_run) {
                    uint8_t operand_bytes = operand_num_bytes(sf_asm, token_buf[i]);
                    if (operand_bytes == 3) {
                        if (check_branch_instruction(sf_asm + token_buf[i - 1].start_index)) {
                            sf->pc++;
                        } else {
                            sf->pc += 2;
                        }
                    } else if (operand_bytes == 0xFF) {
                        fprintf(stderr, "Error at line %d: invalid operand\n", token_buf[i].line_num);
                        exit(ERR_SYNTAX);
                    } else {
                        sf->pc = sf->pc + operand_bytes;
                    }
                    token_buf[i].type = TOKEN_OPERAND;
                }
            }
        }

        if (!directive_run) {
            int i = 0;
            // skip any label declaration tokens
            if (token_buf[0].type == TOKEN_LABEL) {
                i++;
            }
            for (i; i < num_tokens; i++) {
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
Clip_t *assembly_to_clip(sf_t *sf, uint8_t* sf_asm, Table_t **label_table_dbl_ptr) {
    uint8_t *curr_char_ptr = sf_asm;
    uint32_t line_number = 1;
    sf->pc = 0x8000;
    Clip_t *c = new_clip(CLIP_INIT_SIZE);

    while (*curr_char_ptr != '\0') {
        scan_line(sf, c, sf_asm, &curr_char_ptr, line_number, label_table_dbl_ptr);
        line_number++;
    }

    close_roll(c->start + c->index - 1); // close off final roll

    return c;
}
