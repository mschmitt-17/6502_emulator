#include <stdio.h>
#include <stdlib.h>

#include "lib.h"

/* is_uppercase_letter
 *      DESCRIPTION: determines if passed character is uppercase letter or not
 *      INPUTS: curr_char -- character to examine
 *      OUTPUTS: 1 if character is uppercase letter, 0 if character is not uppercase letter
 *      SIDE EFFECTS: none
 */
static uint8_t is_uppercase_letter(uint8_t curr_char) {
    if (curr_char < 0x5B && curr_char > 0x40) {
        return 1;
    }
    return 0;
}

/* is_lowercase_letter
 *      DESCRIPTION: determines if passed character is lowercase letter or not
 *      INPUTS: curr_char -- character to examine
 *      OUTPUTS: 1 if character is lowercase letter, 0 if character is not lowercase letter
 *      SIDE EFFECTS: none
 */
static uint8_t is_lowercase_letter(uint8_t curr_char) {
    if (curr_char < 0x7B && curr_char > 0x60) {
        return 1;
    }
    return 0;
}

/* is_letter
 *      DESCRIPTION: determines if passed character is letter (uppercase or lowercase) or not
 *      INPUTS: curr_char -- character to examine
 *      OUTPUTS: 1 if character is letter, 0 if character is not letter
 *      SIDE EFFECTS: none
 */
uint8_t is_letter(uint8_t curr_char) {
    return (is_uppercase_letter(curr_char) || is_lowercase_letter(curr_char));
}

/* is_decimal_number
 *      DESCRIPTION: determines if passed character is decimal (0-9) number or not
 *      INPUTS: curr_char -- character to examine
 *      OUTPUTS: 1 if character is decimal number, 0 if character is not decimal number
 *      SIDE EFFECTS: none
 */
uint8_t is_decimal_number(uint8_t curr_char) {
    if (curr_char < 0x3A && curr_char > 0x2F) {
        return 1;
    }
    return 0;
}

/* is_hex_number
 *      DESCRIPTION: determines if passed character is hex number (0-F) or not
 *      INPUTS: curr_char -- character to examine
 *      OUTPUTS: 1 if character is hex number, 0 if character is not hex number
 *      SIDE EFFECTS: none
 */
uint8_t is_hex_number(uint8_t curr_char) {
    if ((curr_char < 0x3A && curr_char > 0x2F) ||
        (curr_char < 0x47 && curr_char > 0x40) ||
        (curr_char < 0x67 && curr_char > 0x60)) {
            return 1;
        }
    return 0;
}

/* char_to_hex
 *      DESCRIPTION: converts passed character to hex number
 *      INPUTS: curr_char -- character to be converted
 *      OUTPUTS: converted character if successful, -1 otherwise
 *      SIDE EFFECTS: none
 */
uint8_t char_to_hex(uint8_t curr_char) {
    if (is_decimal_number(curr_char)) {
        return curr_char - 0x30;
    } else if (is_uppercase_letter(curr_char)) {
        return curr_char - 0x37;
    } else if (is_lowercase_letter(curr_char)) {
        return curr_char - 0x57;
    } else {
        return -1;
    }
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

/* validate_label
 *      DESCRIPTION: since labels may only contain select characters (source: ChatGPT), this helper validates all characters in a label
 *      INPUTS: curr_char_ptr -- pointer to beginning character of label
 *              token_len -- number of characters in label
 *      OUTPUTS: 1 if label is valid, 0 if label is invalid
 *      SIDE EFFECTS: none
 */
uint8_t validate_label(uint8_t *curr_char_ptr, uint8_t token_len) {
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
