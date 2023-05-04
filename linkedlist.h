/* =============================================================================
   linkedlist.h

   Linked list implementation that allows any data type to be stored.

   Reference:
   - Taken directly from David Sha's implementation in COMP30023 project 1
   - Implementation of linked list structs inspired by Artem Polyvyanyy from
     ass2-soln-2020.c.

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
list_t *create_empty_list();
list_t *create_list(node_t *head, node_t *foot);
int is_empty_list(list_t *list);
void free_list(list_t *list, void (*free_data)(void *data));
list_t *prepend(list_t *list, void *data);
list_t *append(list_t *list, void *data);
int list_len(list_t *list);
void remove_node(list_t *list, node_t *node);
void *remove_data(list_t *list, void *data);
void *move_data(void *data, list_t *from, list_t *to);
void *pop(list_t *list);
void print_list(list_t *list, void (*print_data)(void *));
list_t *insert_prev(list_t *list, node_t *node, void *data);
list_t *insert_next(list_t *list, node_t *node, void *data);
void *find_node(list_t *list, void *data, int (*cmp)(void *, void *));
int cmp_addr(void *a, void *b);
void copy_list(list_t *from, list_t *to);

#endif
