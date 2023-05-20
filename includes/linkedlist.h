/* =============================================================================
   linkedlist.h

   Linked list implementation that allows any data type to be stored.

   Reference:
   - Taken directly from David Sha's implementation in COMP30023 project 1
   - Originally written for COMP20003 Assignment 2, 2022, altered to fit
     this project.
   - Implementation of linked list structs inspired by Artem Polyvyanyy from
     ass2-soln-2020.c.
   - Implementation of linked list functions inspired by Alistair Moffat
     from "Programming, Problem Solving, and Abstraction with C".

   Author: David Sha
============================================================================= */
#ifndef LINKEDLIST_H
#define LINKEDLIST_H

/* structures =============================================================== */
typedef struct node node_t;
struct node {
    void *data;
    node_t *next;
    node_t *prev;
};

typedef struct list {
    node_t *head;
    node_t *foot;
} list_t;

/* function prototypes ====================================================== */

/*
 * Creates an empty linked list.
 *
 * @return a pointer to the linked list
 */
list_t *create_empty_list();

/*
 * Create a linked list given the head and foot.
 *
 * @param head: the head of the linked list
 * @param foot: the foot of the linked list
 * @return a pointer to the linked list
 */
list_t *create_list(node_t *head, node_t *foot);

/*
 * Checks if list is empty.
 *
 * @param list: the linked list
 */
int is_empty_list(list_t *list);

/*
 * Free the list by freeing all nodes and its contents.
 * Give a function pointer to free the data. If no function
 * pointer is given, the data will not be freed.
 *
 * @param list: the linked list
 * @param free_data: function pointer to free the data
 */
void free_list(list_t *list, void (*free_data)(void *data));

/*
 * Prepend to the list i.e. add to head of linked list.
 *
 * @param list: the linked list
 * @param data: the data to be added
 */
list_t *prepend(list_t *list, void *data);

/*
 * Append to the list i.e. add to foot of linked list.
 *
 * @param list: the linked list
 * @param data: the data to be added
 */
list_t *append(list_t *list, void *data);

/*
 * Get the length of the linked list.
 *
 * @param list: the linked list
 */
int list_len(list_t *list);

/*
 * Remove a node from the linked list.
 *
 * @param list: the linked list
 * @param node: the node address to be removed
 */
void remove_node(list_t *list, node_t *node);

/*
 * Remove data from the linked list. If data is not found,
 * return NULL.
 *
 * @param list: the linked list
 * @param data: the data to be removed
 */
void *remove_data(list_t *list, void *data);

/*
 * Move data from one linked list to another.
 *
 * @param data: the address of the data to be moved
 * @param from: the linked list to move from
 * @param to: the linked list to move to
 */
void *move_data(void *data, list_t *from, list_t *to);

/*
 * Pop the head of the linked list.
 *
 * @param list: the linked list
 */
void *pop(list_t *list);

/*
 * Print the linked list in the format [data1, data2, ...].
 *
 * @param list: the linked list
 * @param print_data: function pointer to print the data
 */
void print_list(list_t *list, void (*print_data)(void *));

/*
 * Insert data before a node.
 *
 * @param list: the linked list
 * @param node: the node to insert before
 * @param data: the data to be inserted
 */
list_t *insert_prev(list_t *list, node_t *node, void *data);

/*
 * Insert data after a node.
 *
 * @param list: the linked list
 * @param node: the node to insert after
 * @param data: the data to be inserted
 */
list_t *insert_next(list_t *list, node_t *node, void *data);

/*
 * Find data in the list. If data is not in the list, return NULL.
 *
 * @param list: the linked list
 * @param data: the data to be found
 * @param cmp: function pointer to compare the data
 * @return the address of the data
 */
void *find_node(list_t *list, void *data, int (*cmp)(void *, void *));

/*
 * Compare two addresses.
 *
 * @param a: the first address
 * @param b: the second address
 * @return 0 if a == b, 1 otherwise
 */
int cmp_addr(void *a, void *b);

/*
 * Copy the data from one list to another. The data is not copied, only
 * the pointers to the data are copied.
 */
void copy_list(list_t *from, list_t *to);

#endif
