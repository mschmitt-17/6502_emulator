#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "table.h"

#define TIME_TO_GROW    0.7
#define GROWTH_FACTOR   2

/*Courtesy of https://www.partow.net/programming/hashfunctions/*/
/* RSHash
 *      DESCRIPTION: general purpose hash function
 *      INPUTS: str -- string to hash
 *              length -- length of string to hash
 *      OUTPUTS: hashed value
 *      SIDE EFFECTS: none
 */
static uint32_t RSHash(const char* str, uint32_t length) {
    uint32_t b = 378551;
    uint32_t a = 63689;
    uint32_t hash = 0;

    for (uint32_t i = 0; i < length; str++, i++){
        hash = hash * a + (*str);
        a = a * b;
    }

    return hash;
}

/*Also courtesy of https://www.partow.net/programming/hashfunctions/*/
/* JSHash
 *      DESCRIPTION: another general purpose hash function
 *      INPUTS: str -- string to hash
 *              length -- length of string to hash
 *      OUTPUTS: hashed value
 *      SIDE EFFECTS: none
 */
static uint32_t JSHash(const char* str, uint32_t length) {
    uint32_t hash = 1315423911;

    for (uint32_t i = 0; i < length; str++, i++) {
        hash ^= ((hash << 5) + (*str) + (hash >> 2));
    }

    return hash;
}

/* initialize_table
 *      DESCRIPTION: sets all key-value pairs in table to known initial values
 *      INPUTS: t -- table we wish to initialize
 *      OUTPUTS: none
 *      SIDE EFFECTS: resets all key-value pairs in table to initial value
 */
static void initialize_table(Table_t *t) {
    for (uint32_t i = 0; i < t->size; i++) {
        t->data[i].key = NULL;
        t->data[i].value = 0x0000;
    }
}

/* copy_table
 *      DESCRIPTION: rehashes all keys from one table to another table
 *      INPUTS: to -- table to copy all data to
 *              from -- table to copy all data from
 *      OUTPUTS: none
 *      SIDE EFFECTS: adds data from to to from
 */
static void copy_table(Table_t* to, Table_t *from) {
    for (uint32_t i = 0; i < to->size; i++) {
        if (to->data[i].key != NULL) {
            add_to_table(&from, to->data[i].key, to->data[i].value);
        }
    }
}

/* free_table
 *      DESCRIPTION: frees table and associated data structures
 *      INPUTS: t -- table to free
 *      OUTPUTS: none
 *      SIDE EFFECTS: frees passed table
 */
void free_table(Table_t *t) {
    for (uint32_t i = 0; i < t->size; i++) {
        if (t->data[i].key != NULL) {
            free(t->data[i].key);
        }
    }
    free(t->data);
    free(t);
}

/* new_table
 *      DESCRIPTION: allocates memory for and initializes new table
 *      INPUTS: size -- size of table to allocate
 *      OUTPUTS: pointer to newly allocated table
 *      SIDE EFFECTS: none
 */
Table_t *new_table(uint32_t size) {
    Table_t *new_t = (Table_t *)malloc(sizeof(Table_t));
    new_t->size = size;
    new_t->occupied = 0;
    new_t->data = (Entry_t *)malloc(sizeof(Entry_t) * size);
    initialize_table(new_t);
    return new_t;
}

/* add_to_table
 *      DESCRIPTION: hashes passed key and adds corresponding value to table (using double hashing)
 *      INPUTS: t_dbl_ptr -- double pointer to table we wish to add to
 *              key -- string to hash
 *              value -- value to store in table with hashed string
 *      OUTPUTS: -1 -- value was already present in table
 *                0 -- value was not already present in table
 *      SIDE EFFECTS: adds new key-value pair to table
 */
int add_to_table(Table_t **t_dbl_ptr, char *key, uint16_t value) {
    if (((float)(*t_dbl_ptr)->occupied)/(*t_dbl_ptr)->size >= TIME_TO_GROW) {
        Table_t *new_t = new_table((*t_dbl_ptr)->size * GROWTH_FACTOR);
        copy_table(*t_dbl_ptr, new_t);
        free_table(*t_dbl_ptr);
        *t_dbl_ptr = new_t;
    }

    uint32_t index = RSHash(key, strlen(key)) % (*t_dbl_ptr)->size;
    while ((*t_dbl_ptr)->data[index].key != NULL) {
        if (!(strcmp((*t_dbl_ptr)->data[index].key, key))) {
            return -1;
        }
        index = (index + JSHash(key, strlen(key))) % (*t_dbl_ptr)->size;
    }

    (*t_dbl_ptr)->data[index].key = (char *)malloc((strlen(key) + 1) * sizeof(char));
    strcpy((*t_dbl_ptr)->data[index].key, key);
    (*t_dbl_ptr)->data[index].value = value;
    (*t_dbl_ptr)->occupied++;
    
    return 0;
}

/* get_value
 *      DESCRIPTION: retrieves value corresponding to passed key
 *      INPUTS: t -- table we wish to retrieve value from
 *              key -- string whose corresponding value we want to retrieve
 *      OUTPUTS: value corresponding to passed key
 *      SIDE EFFECTS: will stay in infinite loop if key is not present in table
 */
uint16_t get_value(Table_t *t, char *key) {
    uint32_t index = RSHash(key, strlen(key)) % t->size;
    while (strcmp(t->data[index].key, key)) {
        index = (index + JSHash(key, strlen(key))) % t->size;
    }
    return t->data[index].value;
}
