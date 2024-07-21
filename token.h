#ifndef __TOKEN_H
#define __TOKEN_H

#include <stdint.h>

typedef enum {
    TOKEN_OPERAND,
    TOKEN_INSTRUCTION,
    TOKEN_LABEL
} TokenType_t;

typedef struct {
    TokenType_t type;
    uint32_t start_index;
    uint32_t end_index;
    uint32_t line_num;
} Token_t;

typedef struct {
    Token_t *start;
    uint16_t start_address;
    uint32_t index;
    uint32_t size;
} Roll_t;

typedef struct {
    Roll_t *start;
    uint32_t index;
    uint32_t size;
} Clip_t;

void add_to_roll(Roll_t *r, Token_t *t);
Clip_t* new_clip(uint32_t size);
void free_clip(Clip_t *c);
void open_roll(Clip_t *c, uint16_t start_address, uint32_t line_num);

#endif
