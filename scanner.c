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

static void run_directive(Clip_t *c, uint8_t *start, Token_t directive_token, Token_t operand_token) {
    if (directive_token.end_index - directive_token.start_index + 1 == 4) {
        // directives only take absolute addressing
        if (operand_token.end_index - operand_token.start_index + 1 == 5 && *(start + operand_token.start_index) == '$') {
            if (*(start + directive_token.start_index + 1) == 'O' || *(start + directive_token.start_index + 1) == 'o') {
                if (*(start + directive_token.start_index + 2) == 'R' || *(start + directive_token.start_index + 2) == 'r') {
                    if (*(start + directive_token.start_index + 3) == 'G' || *(start + directive_token.start_index + 3) == 'g') {

                        uint16_t load_address = 0x0000;
                        
                        for (int i = 0; i < 4; i++) {
                            if (!is_hex_number(*(start + operand_token.start_index + i + 1))) {
                                fprintf(stderr, "Invalid operand at line %d\n", operand_token.line_num);
                                exit(ERR_INVALID_OPERAND_OPCODE);
                            }
                            load_address |= (char_to_hex(*(start + operand_token.start_index + i + 1))) << i * 4;
                        }
                        
                        open_roll(c, load_address, directive_token.line_num);
                        return;
                    }
                }
            }
        }
    }
    fprintf(stderr, "Invalid directive at line %d\n", directive_token.line_num);
    exit(ERR_INVALID_DIRECTIVE);
}

/* read_file
 *      DESCRIPTION: reads passed file into buffer
 *      INPUTS: file_path -- path to file to be read into buffer
 *      OUTPUTS: buffer which file has been read into
 *      SIDE EFFECTS: allocates memory for buffer which file is read into
 */
static uint8_t *read_file(const char *file_path) {
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

static void scan_line(Clip_t *c, uint8_t* start, uint8_t **curr_char_dbl_ptr, uint32_t line_number) {

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
            uint32_t token_start_index = *curr_char_dbl_ptr - start;
            uint32_t token_end_index = *curr_char_dbl_ptr - start + skip_until_whitespace(curr_char_dbl_ptr) - 1;
            token_buf[num_tokens].start_index = token_start_index;
            token_buf[num_tokens].end_index = token_end_index;
            token_buf[num_tokens].line_num = line_number;
            num_tokens++;
            skip_whitespace(curr_char_dbl_ptr);
        }

        uint8_t curr_token = 0;
        if (num_tokens) {
            // INSTRUCTION
            if (num_tokens == 1) {
                token_buf[curr_token++].type = TOKEN_INSTRUCTION;
            } else {
                // if we have 3 tokens, we know the first one has to be a label
                if (num_tokens == 3) {
                    token_buf[curr_token++].type = TOKEN_LABEL;
                }

                // if final token is 3 characters long and the first character is a letter, then we need to have LABEL INSTRUCTION
                if (token_buf[curr_token + 1].end_index - token_buf[curr_token + 1].start_index + 1 == 3 &&
                        is_letter(start[token_buf[curr_token + 1].start_index])) {
                    token_buf[curr_token++].type = TOKEN_LABEL;
                    token_buf[curr_token++].type = TOKEN_INSTRUCTION;
                } 
                
                // if the above is not true, we may have ASSEMBLER-DIRECTIVE OPERAND or INSTRUCTION OPERAND
                else {
                    if (start[token_buf[curr_token].start_index] == '.') {
                        run_directive(c, start, token_buf[curr_token], token_buf[curr_token + 1]);
                        directive_run = 1;
                    } else {
                        token_buf[curr_token++].type = TOKEN_INSTRUCTION;
                        token_buf[curr_token++].type = TOKEN_OPERAND;
                    }
                }
            } 
        }

        if (!directive_run) {
            // add tokens to roll
            for (int i = 0; i < curr_token; i++) {
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

Clip_t *assembly_to_clip(const char *file_path) {
    uint8_t *sf_asm = read_file(file_path);
    uint8_t *curr_char_ptr = sf_asm;
    uint32_t line_number = 1;
    Clip_t *c = new_clip(CLIP_INIT_SIZE);

    while (*curr_char_ptr != '\0') {
        scan_line(c, sf_asm, &curr_char_ptr, line_number);
        line_number++;
    }

    free(sf_asm);

    return c;
}
