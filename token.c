#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lib.h"
#include "token.h"

#define CLIP_GROWTH_FACTOR      2
#define ROLL_GROWTH_FACTOR      2
#define ROLL_INIT_SIZE          256
#define ROM_START               0x8000

static void init_roll(Roll_t *r, uint32_t size, uint16_t start_address) {
    r->start = (Token_t *)malloc(size * sizeof(Token_t));
    r->index = 0;
    r->size = size;
    r->start_address = start_address;
}

static int expand_roll(Roll_t *r) {
    r->start = (Token_t *)realloc(r->start, r->size * ROLL_GROWTH_FACTOR);
    if (r->start == NULL) {
        return -1;
    }
    r->size *= ROLL_GROWTH_FACTOR;
    return 0;
}

void add_to_roll(Roll_t *r, Token_t *t) {
    if (r->index >= r->size) {
        if (expand_roll(r) == -1) {
            fprintf(stderr, "Error at line %d: roll memory allocation failed", t->line_num);
            exit(ERR_NO_MEM);
        }
    }
    memcpy(r->start + r->index, t, sizeof(Token_t));
    r->index++;
}

Clip_t* new_clip(uint32_t size) {
    Clip_t *c = (Clip_t *)malloc(sizeof(Clip_t));
    c->start = (Roll_t *)malloc(size * sizeof(Roll_t));
    init_roll(c->start, ROLL_INIT_SIZE, ROM_START);
    c->index = 1;
    c->size = size;
    return c;
}

void free_clip(Clip_t *c) {
    for (int i = 0; i < c->index; i++) {
        free(c->start[i].start);
    }
    free(c->start); 
    free(c);
}

static int expand_clip(Clip_t *c) {
    c->start = (Roll_t *)realloc(c->start, c->size * CLIP_GROWTH_FACTOR);
    if (c->start == NULL) {
        return -1;
    }
    c->size *= CLIP_GROWTH_FACTOR;
    return 0;
}

void open_roll(Clip_t *c, uint16_t start_address, uint32_t line_num) {
    if (c->index >= c->size) {
        if (expand_clip(c) == -1) {
            fprintf(stderr, "Error at line %d: allocation for new memory region failed", line_num);
            exit(ERR_NO_MEM);
        }
    }
    init_roll(c->start + c->index, ROLL_INIT_SIZE, start_address);
    c->index++;
}