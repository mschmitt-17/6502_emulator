#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../lib/lib.h"
#include "bytecode.h"

#define BYTECODE_GROWTH_FACTOR  2
#define PROGRAM_GROWTH_FACTOR   2
#define BYTECODE_INIT_SIZE      256

/* init_bytecode
 *      DESCRIPTION: initializes passed bytecode with passed parameters
 *      INPUTS: bc -- bytecode to initialize
 *              load_address -- initial value for bytecode's load address
 *              size -- initial value for bytecode's size
 *      OUTPUTS: none
 *      SIDE EFFECTS: modifies fields of passed bytecode
 */
static void init_bytecode(Bytecode_t *bc, uint16_t load_address, uint32_t size) {
    bc->start = (uint8_t *)malloc(size);
    bc->load_address = load_address;
    bc->index = 0;
    bc->size = size;
}

/* expand_bytecode
 *      DESCRIPTION: increases size of passed bytecode by a factor of BYTECODE_GROWTH_FACTOR, keeping preexisting bytecode intact
 *      INPUTS: bc -- bytecode to expand
 *      OUTPUTS: -1 if size increase fails, 0 if success
 *      SIDE EFFECTS: increases size of passed bytecode
 */
static int expand_bytecode(Bytecode_t *bc) {
    bc->start = (uint8_t *)realloc(bc->start, bc->size * BYTECODE_GROWTH_FACTOR);
    if (bc->start == NULL) {
        return -1;
    }
    bc->size *= BYTECODE_GROWTH_FACTOR;
    return 0;
}

/* add_to_bytecode
 *      DESCRIPTION: copies bytes into bytecode, expanding if necessary
 *      INPUTS: bc -- bytecode to add new bytes to
 *              write_buf -- buffer containing new bytes to be added to bytecode
 *              num_bytes -- number of bytes to write to bytecode
 *              line_number -- line number of assembly being added to bytecode (used for error message)
 *      OUTPUTS: none
 *      SIDE EFFECTS: adds bytes to bytecode, increases index field of bytecode
 */
void add_to_bytecode(Bytecode_t *bc, uint8_t *write_buf, uint32_t num_bytes, uint32_t line_number) {
    if ((num_bytes + bc->index) >= bc->size - 1) {
        if (expand_bytecode(bc) == -1) {
            fprintf(stderr, "Error at line %d: bytecode memory allocation failed", line_number);
            exit(ERR_NO_MEM);
        }
    }
    memcpy(bc->start + bc->index, write_buf, num_bytes);
    bc->index += num_bytes;
}

/* new_program
 *      DESCRIPTION: creates new program of passed size
 *      INPUTS: initial number of pieces of bytecode for new program
 *      OUTPUTS: p -- new program
 *      SIDE EFFECTS: allocates memory for new program, initializes fields of new program
 */
Program_t* new_program(uint32_t size) {
    Program_t *p = (Program_t *)malloc(sizeof(Program_t));
    p->start = (Bytecode_t *)malloc(size * sizeof(Bytecode_t));
    p->index = 0;
    p->size = size;
    return p;
}

/* free_program
 *      DESCRIPTION: frees memory allocated for passed program
 *      INPUTS: p -- program to free
 *      OUTPUTS: none
 *      SIDE EFFECTS: frees memory allocated for passed program
 */
void free_program(Program_t *p) {
    for (int i = 0; i < p->index; i++) {
        free(p->start[i].start);
    }
    free(p->start);
    free(p);
}

/* expand_program
 *      DESCRIPTION: increases size of passed program by a factor of PROGRAM_GROWTH_FACTOR, keeping preexisting program fields intact
 *      INPUTS: p -- program to expand
 *      OUTPUTS: -1 if size increase fails, 0 if success
 *      SIDE EFFECTS: increases size of passed program
 */
static int expand_program(Program_t *p) {
    p->start = (Bytecode_t *)realloc(p->start, p->size * PROGRAM_GROWTH_FACTOR);
    if (p->start == NULL) {
        return -1;
    }
    p->size *= PROGRAM_GROWTH_FACTOR;
    return 0;
}

/* open_bytecode
 *      DESCRIPTION: initializes the next piece of bytecode in the passed program
 *      INPUTS: p -- program to initialize the next piece of bytecode in
 *              load_address -- load address to initialize piece of bytecode with
 *              line_number -- line number of current line being generated (used for error message)
 *      OUTPUTS: none
 *      SIDE EFFECTS: increments index field of program, initializes next piece of bytecode in program
 */
void open_bytecode(Program_t *p, uint16_t load_address, uint32_t line_number) {
    if (p->index >= p->size - 1) {
        if (expand_program(p) == -1) {
            fprintf(stderr, "Error at line %d: program memory allocation failed", line_number);
            exit(ERR_NO_MEM);
        }
    }
    init_bytecode(p->start + p->index, load_address, BYTECODE_INIT_SIZE);
    p->index++;
}
