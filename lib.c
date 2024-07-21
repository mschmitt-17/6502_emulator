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
