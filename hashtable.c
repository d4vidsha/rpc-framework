/* =============================================================================
   hashtable.c

   Hash table implementation that allows any data type to be stored.

   Author: David Sha
============================================================================= */
#include "hashtable.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

unsigned long hash(unsigned char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

hashtable_t *hashtable_create(int size) {
    assert(size > 0);
    hashtable_t *new = (hashtable_t *)malloc(sizeof(*new));
    assert(new);
    new->table = (item_t **)calloc(size, sizeof(item_t *));
    assert(new->table);
    new->size = size;
    return new;
}

void hashtable_destroy(hashtable_t *hashtable, void (*free_data)(void *data)) {
    assert(hashtable);
    for (int i = 0; i < hashtable->size; i++) {
        if (hashtable->table[i]) {
            free_list(hashtable->table[i]->data, free_data);
            free(hashtable->table[i]);
        }
    }
    free(hashtable->table);
    free(hashtable);
}

void hashtable_insert(hashtable_t *hashtable, unsigned long key, void *data) {
    assert(hashtable);
    int index = key % hashtable->size;
    if (hashtable->table[index]) {
        hashtable->table[index]->data = prepend(hashtable->table[index]->data, data);
    } else {
        item_t *new = (item_t *)malloc(sizeof(*new));
        assert(new);
        new->key = key;
        new->data = create_empty_list();
        new->data = prepend(new->data, data);
        hashtable->table[index] = new;
    }
}

item_t *hashtable_search(hashtable_t *hashtable, unsigned long key) {
    assert(hashtable);
    int index = key % hashtable->size;
    if (hashtable->table[index]) {
        return hashtable->table[index];
    }
    return NULL;
}

void hashtable_remove(hashtable_t *hashtable, unsigned long key, void (*free_data)(void *)) {
    assert(hashtable);
    int index = key % hashtable->size;
    if (hashtable->table[index]) {
        free_list(hashtable->table[index]->data, free_data);
        free(hashtable->table[index]);
        hashtable->table[index] = NULL;
    }
}

void hashtable_print(hashtable_t *hashtable, void (*print_data)(void *)) {
    assert(hashtable);
    for (int i = 0; i < hashtable->size; i++) {
        if (hashtable->table[i]) {
            printf("%d: ", i);
            print_list(hashtable->table[i]->data, print_data);
        }
    }
}