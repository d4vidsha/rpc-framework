/* =============================================================================
   linkedlist.c

   Linked list implementation that allows any data type to be stored.

   Reference:
   - Taken directly from David Sha's implementation in COMP30023 project 1
   - Originally written for COMP20003 Assignment 2, 2022, altered to fit
     this project.
   - Implementation of linked list functions inspired by Alistair Moffat
     from "Programming, Problem Solving, and Abstraction with C".

   Author: David Sha
============================================================================= */
#include "linkedlist.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

list_t *create_empty_list() {
    /*  Creates an empty linked list.
     */
    list_t *list;
    list = (list_t *)malloc(sizeof(*list));
    assert(list);
    list->head = list->foot = NULL;
    return list;
}

list_t *create_list(node_t *head, node_t *foot) {
    /*  Create a linked list given the head and foot.
     */
    assert(head && foot);
    list_t *new = create_empty_list();
    new->head = head;
    new->foot = foot;
    return new;
}

int is_empty_list(list_t *list) {
    /*  Checks if list is empty.
     */
    assert(list);
    return list->head == NULL;
}

void free_list(list_t *list, void (*free_data)(void *data)) {
    /*  Free the list by freeing all nodes and its contents.
        Give a function pointer to free the data. If no function
        pointer is given, the data will not be freed.
     */
    assert(list);
    node_t *curr, *prev;
    curr = list->head;
    while (curr) {
        prev = curr;
        curr = curr->next;
        if (free_data) {
            free_data(prev->data);
        }
        free(prev);
    }
    free(list);
}

list_t *prepend(list_t *list, void *data) {
    /*  Prepend to the list i.e. add to head of linked list.
     */
    assert(list && data);
    node_t *new;
    new = (node_t *)malloc(sizeof(*new));
    assert(new);
    new->data = data;
    new->prev = NULL;
    new->next = list->head;
    if (list->head) {
        list->head->prev = new;
    } else {
        /* this is the first insert into list */
        list->foot = new;
    }
    list->head = new;
    return list;
}

list_t *append(list_t *list, void *data) {
    /*  Append to the list i.e. add to foot of linked list.
     */
    assert(list && data);
    node_t *new;
    new = (node_t *)malloc(sizeof(*new));
    assert(new);
    new->data = data;
    new->next = NULL;
    new->prev = list->foot;
    if (list->foot) {
        list->foot->next = new;
    } else {
        /* this is the first insert into list */
        list->head = new;
    }
    list->foot = new;
    return list;
}

int list_len(list_t *list) {
    /*  Get the linked list length.
     */
    assert(list);
    int len = 0;
    node_t *curr;
    curr = list->head;
    while (curr) {
        len++;
        curr = curr->next;
    }
    return len;
}

void remove_node(list_t *list, node_t *node) {
    /*  Remove a node from the list.
     */
    assert(list && node);
    if (node->prev == NULL) {
        /* removing the first node in the list */
        list->head = node->next;
        if (list->foot == node) {
            /* also removing the last node in the list */
            list->foot = NULL;
        } else {
            /* set the prev pointer of the new head */
            list->head->prev = NULL;
        }
    } else {
        /* removing a node in the middle or end of the list */
        node->prev->next = node->next;
        if (list->foot == node) {
            /* also removing the last node in the list */
            list->foot = node->prev;
        } else {
            /* set the prev pointer of the next node */
            node->next->prev = node->prev;
        }
    }
    free(node);
}

void *remove_data(list_t *list, void *data) {
    /*  Remove data from the list. If data is not in the list, return NULL.
     */
    assert(list && data);
    node_t *curr;
    curr = list->head;
    while (curr) {
        if (curr->data == data) {
            remove_node(list, curr);
            return data;
        }
        curr = curr->next;
    }
    return NULL;
}

void *move_data(void *data, list_t *from, list_t *to) {
    /*  Move data from one list to another.
     */
    assert(data && from && to);
    void *removed = remove_data(from, data);
    if (removed) {
        append(to, removed);
    }
    return removed;
}

void *pop(list_t *list) {
    /*  Pop from the head of list.
     */
    assert(list);
    node_t *head = list->head;
    if (head) {
        list->head = head->next;
        if (list->foot == head) {
            /* also removing the last node in the list */
            list->foot = NULL;
        } else {
            /* set the prev pointer of the new head */
            list->head->prev = NULL;
        }
        void *data = head->data;
        free(head);
        return data;
    }
    return NULL;
}

void print_list(list_t *list, void (*print_data)(void *)) {
    /*  Print a linked list in the format [data1, data2, ...].
     */
    assert(list);
    printf("[");
    for (node_t *curr = list->head; curr; curr = curr->next) {
        print_data(curr->data);
        if (curr->next) {
            printf(", ");
        }
    }
    printf("]\n");
}

list_t *insert_prev(list_t *list, node_t *node, void *data) {
    /*  Insert data before the given node.
     */
    assert(list && node && data);
    node_t *new;
    new = (node_t *)malloc(sizeof(*new));
    assert(new);
    new->data = data;
    new->prev = node->prev;
    new->next = node;
    if (node->prev) {
        node->prev->next = new;
    } else {
        /* this is the first insert into list */
        list->head = new;
    }
    node->prev = new;
    return list;
}

list_t *insert_next(list_t *list, node_t *node, void *data) {
    /*  Insert data after the given node.
     */
    assert(list && node && data);
    node_t *new;
    new = (node_t *)malloc(sizeof(*new));
    assert(new);
    new->data = data;
    new->next = node->next;
    new->prev = node;
    if (node->next) {
        node->next->prev = new;
    } else {
        /* this is the first insert into list */
        list->foot = new;
    }
    node->next = new;
    return list;
}

void *find_node(list_t *list, void *data, int (*cmp)(void *, void *)) {
    /*  Find data in the list. If data is not in the list, return NULL.
     */
    assert(list && data && cmp);
    for (node_t *curr = list->head; curr; curr = curr->next) {
        if (cmp(curr->data, data) == 0) {
            return curr;
        }
    }
    return NULL;
}

int cmp_addr(void *a, void *b) {
    /*  Compare two addresses. If a == b, return 0. Otherwise, return 1.
     */
    return a != b;
}

void copy_list(list_t *from, list_t *to) {
    /*  Copy the data from one list to another. The data is not copied, only
        the pointers to the data are copied.
     */
    for (node_t *curr = from->head; curr; curr = curr->next) {
        void *data = curr->data;
        append(to, data);
    }
}
