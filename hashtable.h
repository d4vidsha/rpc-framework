/* =============================================================================
   hashtable.h

   Hash table implementation that allows any data type to be stored.

   Reference:
   - Inspired by DigitalOcean's implementation of a hash table in C++.
     https://www.digitalocean.com/community/tutorials/hash-table-in-c-plus-plus

   Author: David Sha
============================================================================= */
#ifndef HASHTABLE_H
#define HASHTABLE_H

/* structures =============================================================== */
typedef struct item item_t;
struct item {
    const char *key;
    void *data;
    item_t *next;
};

typedef struct hashtable {
    item_t **table;
    int size;
} hashtable_t;


/* function prototypes ====================================================== */

/*
 * Hash function which is djb2 by Dan Bernstein.
 * 
 * @param str The string to hash.
 * @return The hash value.
 * @note See more on djb2 at https://theartincode.stanis.me/008-djb2/
 */
unsigned long hash(const char *str);

/*
 * Creates a new hashtable.
 * 
 * @param size The size of the hashtable.
 * @return A pointer to the hashtable, or NULL on failure.
 */
hashtable_t *hashtable_create(int size);

/*
 * Destroys a hashtable.
 * 
 * @param hashtable The hashtable to destroy.
 * @param free_data The function to free the data of each item. NULL if no
 * freeing is required.
 */
void hashtable_destroy(hashtable_t *hashtable, void (*free_data)(void *));

/*
 * Insert an item to the hashtable. If the key already exists, the data will
 * be inserted to the front of the linked list.
 * 
 * @param hashtable The hashtable to insert the item to.
 * @param key The key of the item.
 * @param data The data of the item.
 */
void hashtable_insert(hashtable_t *hashtable, const char *key, void *data);

/*
 * Lookup an item in the hashtable by key.
 * 
 * @param hashtable The hashtable to search.
 * @param key The key of the item.
 * @return A pointer to the item, or NULL if not found.
 */
void *hashtable_lookup(hashtable_t *hashtable, const char *key);

/*
 * Remove an item from the hashtable.
 * 
 * @param hashtable The hashtable to remove the item from.
 * @param key The key of the item.
 * @param free_data The function to free the data of the item. NULL if no
 * freeing is required.
 */
void hashtable_remove(hashtable_t *hashtable, const char *key, void (*free_data)(void *));

/*
 * Prints the hashtable.
 * 
 * @param hashtable The hashtable to print.
 */
void hashtable_print(hashtable_t *hashtable, void (*print_data)(void *));

#endif
