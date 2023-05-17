/* =============================================================================
   hashtable.c

   Hash table implementation that allows any data type to be stored.

   Author: David Sha
============================================================================= */
#include "hashtable.h"
#include "config.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned long hash(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

hashtable_t *hashtable_create(int size) {
    assert(size > 0);
    hashtable_t *hashtable;
    hashtable = (hashtable_t *)malloc(sizeof(*hashtable));
    assert(hashtable);
    hashtable->table = (item_t **)malloc(sizeof(item_t *) * size);
    assert(hashtable->table);
    for (int i = 0; i < size; i++) {
        hashtable->table[i] = NULL;
    }
    hashtable->size = size;
    return hashtable;
}

void hashtable_destroy(hashtable_t *hashtable, void (*free_data)(void *)) {
    assert(hashtable);
    for (int i = 0; i < hashtable->size; i++) {
        item_t *curr = hashtable->table[i], *prev = NULL;
        while (curr) {
            prev = curr;
            curr = curr->next;
            hashtable_item_free(prev, free_data);
        }
    }
    free(hashtable->table);
    free(hashtable);
}

void hashtable_insert(hashtable_t *hashtable, char *key, void *data) {
    assert(hashtable && key && data);
    unsigned long index = hash(key) % hashtable->size;
    item_t *new = (item_t *)malloc(sizeof(*new));
    assert(new);
    new->key = (char *)malloc(strlen(key) + 1);
    assert(new->key);
    strcpy((char *)new->key, key);
    new->data = data;
    new->next = hashtable->table[index];
    hashtable->table[index] = new;
}

void *hashtable_lookup(hashtable_t *hashtable, char *key) {
    assert(hashtable && key);
    unsigned long index = hash(key) % hashtable->size;
    item_t *curr = hashtable->table[index];
    int cmp;
    debug_print("Looking up hashtable[%lu]\n", index);
    while (curr) {
        cmp = strcmp(curr->key, key);
        debug_print("%s<->%s = %d\n", curr->key, key, cmp);
        if (cmp == 0) {
            return curr->data;
        }
        curr = curr->next;
    }
    return NULL;
}

void hashtable_remove(hashtable_t *hashtable, char *key,
                      void (*free_data)(void *)) {
    assert(hashtable && key);
    unsigned long index = hash(key) % hashtable->size;
    item_t *curr = hashtable->table[index], *prev = NULL;
    while (curr) {
        if (strcmp(curr->key, key) == 0) {
            if (prev) {
                prev->next = curr->next;
            } else {
                hashtable->table[index] = curr->next;
            }
            hashtable_item_free(curr, free_data);
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

void hashtable_item_free(item_t *item, void (*free_data)(void *)) {
    assert(item);
    free((char *)item->key);
    if (free_data) {
        free_data(item->data);
    }
    free(item);
}

void hashtable_print(hashtable_t *hashtable, void (*print_data)(void *)) {
    assert(hashtable);
    for (int i = 0; i < hashtable->size; i++) {
        item_t *curr = hashtable->table[i];
        while (curr) {
            printf("%s: ", curr->key);
            if (print_data) {
                print_data(curr->data);
            }
            printf("\n");
            curr = curr->next;
        }
    }
}
