#ifndef __TABLE_H
#define __TABLE_H

#include <stdint.h>

typedef struct Entry {
    char *key;
    uint16_t value;
} Entry_t;

typedef struct Table {
    Entry_t *data;
    uint32_t size;
    uint32_t occupied;
} Table_t;

Table_t *new_table(uint32_t size);
void free_table(Table_t *t);
int add_to_table(Table_t **t_dbl_ptr, char *key, uint16_t value);
uint16_t get_value(Table_t *t, char *key);

#endif
