#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lib.h"
#include "token.h"

#define CLIP_GROWTH_FACTOR      2
#define ROLL_GROWTH_FACTOR      2
#define ROLL_INIT_SIZE          256

/* init_roll
 *      DESCRIPTION: initializes passed roll with passed parameters
 *      INPUTS: r -- roll to initialize
 *              start_address -- initial value for roll's start address
 *              size -- initial value for roll's size
 *      OUTPUTS: none
 *      SIDE EFFECTS: modifies fields of passed roll
 */
static void init_roll(Roll_t *r, uint16_t start_address, uint32_t size) {
    r->start = (Token_t *)malloc(size * sizeof(Token_t));
    r->index = 0;
    r->start_address = start_address;
    r->size = size;
}

/* expand_roll
 *      DESCRIPTION: increases size of passed roll by a factor of ROLL_GROWTH_FACTOR, keeping preexisting roll intact
 *      INPUTS: r -- roll to expand
 *      OUTPUTS: -1 if size increase fails, 0 if success
 *      SIDE EFFECTS: increases size of passed roll
 */
static int expand_roll(Roll_t *r) {
    r->start = (Token_t *)realloc(r->start, r->size * ROLL_GROWTH_FACTOR);
    if (r->start == NULL) {
        return -1;
    }
    r->size *= ROLL_GROWTH_FACTOR;
    return 0;
}

/* add_to_roll
 *      DESCRIPTION: copies tokens into roll, expanding if necessary
 *      INPUTS: r -- roll to add new tokens to
 *              t -- token to add to roll
 *      OUTPUTS: none
 *      SIDE EFFECTS: adds token to roll, increases index field of roll
 */
void add_to_roll(Roll_t *r, Token_t *t) {
    if (r->index >= r->size - 1) {
        if (expand_roll(r) == -1) {
            fprintf(stderr, "Error at line %d: roll memory allocation failed", t->line_num);
            exit(ERR_NO_MEM);
        }
    }
    memcpy(r->start + r->index, t, sizeof(Token_t));
    r->index++;
}

/* close_roll
 *      DESCRIPTION: adds end token to roll
 *      INPUTS: r -- roll to add end token to
 *      OUTPUTS: none
 *      SIDE EFFECTS: adds end token to roll
 */
void close_roll(Roll_t *r) {
    Token_t end_token;
    if (r->index == 0) {
        end_token.line_num = 0;
    } else {
        end_token.line_num = r->start[r->index - 1].line_num; 
    }
    end_token.type = TOKEN_END;
    add_to_roll(r, &end_token);
}

/* new_clip
 *      DESCRIPTION: creates new clip of passed size
 *      INPUTS: initial number of rolls for new clip
 *      OUTPUTS: c -- new clip
 *      SIDE EFFECTS: allocates memory for new clip, initializes fields of new clip
 */
Clip_t* new_clip(uint32_t size) {
    Clip_t *c = (Clip_t *)malloc(sizeof(Clip_t));
    c->start = (Roll_t *)malloc(size * sizeof(Roll_t));
    init_roll(c->start, ROLL_INIT_SIZE, ROM_START);
    c->index = 1;
    c->size = size;
    return c;
}

/* free_clip
 *      DESCRIPTION: frees memory allocated for passed clip
 *      INPUTS: c -- clip to free
 *      OUTPUTS: none
 *      SIDE EFFECTS: frees memory allocated for passed clip
 */
void free_clip(Clip_t *c) {
    for (int i = 0; i < c->index; i++) {
        free(c->start[i].start);
    }
    free(c->start); 
    free(c);
}

/* expand_clip
 *      DESCRIPTION: increases size of passed clip by a factor of CLIP_GROWTH_FACTOR, keeping preexisting clip fields intact
 *      INPUTS: c -- clip to expand
 *      OUTPUTS: -1 if size increase fails, 0 if success
 *      SIDE EFFECTS: increases size of passed clip
 */
static int expand_clip(Clip_t *c) {
    c->start = (Roll_t *)realloc(c->start, c->size * CLIP_GROWTH_FACTOR);
    if (c->start == NULL) {
        return -1;
    }
    c->size *= CLIP_GROWTH_FACTOR;
    return 0;
}

/* open_roll
 *      DESCRIPTION: initializes the next roll in the passed clip
 *      INPUTS: c -- clip to initialize the next roll in
 *              start_address -- start address to initialize roll with
 *              line_num -- line number of current line being scanned (used for error message)
 *      OUTPUTS: none
 *      SIDE EFFECTS: increments index field of clip, initializes next roll in program
 */
void open_roll(Clip_t *c, uint16_t start_address, uint32_t line_num) {
    if (c->index >= c->size - 1) {
        if (expand_clip(c) == -1) {
            fprintf(stderr, "Error at line %d: allocation for new memory region failed", line_num);
            exit(ERR_NO_MEM);
        }
    }
    init_roll(c->start + c->index, ROLL_INIT_SIZE, start_address);
    c->index++;
}
