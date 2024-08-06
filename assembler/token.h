#ifndef __TOKEN_H
#define __TOKEN_H

#include <stdint.h>

// type of token
typedef enum {
    TOKEN_EMPTY = 0,
    TOKEN_PENDING,
    TOKEN_OPERAND,
    TOKEN_INSTRUCTION,
    TOKEN_LABEL,
    TOKEN_END
} TokenType_t;

// token indexing specific characters in assembly
typedef struct {
    TokenType_t type;
    uint32_t start_index;
    uint32_t end_index;
    uint32_t line_num;
} Token_t;

// an expandable container of tokens
typedef struct {
    Token_t *start;
    uint16_t start_address;
    uint32_t index;
    uint32_t size;
} Roll_t;

// an expandable container of rolls
typedef struct {
    Roll_t *start;
    uint32_t index;
    uint32_t size;
} Clip_t;

void add_to_roll(Roll_t *r, Token_t *t);
void close_roll(Roll_t *r);
Clip_t* new_clip(uint32_t size);
void free_clip(Clip_t *c);
void open_roll(Clip_t *c, uint16_t start_address, uint32_t line_num);

#endif
