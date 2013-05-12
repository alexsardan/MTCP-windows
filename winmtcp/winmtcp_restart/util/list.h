#ifndef SLIST_H
#define SLIST_H

#include <Windows.h>

typedef struct node
{
	PVOID data;
    struct node* next;
} node_t;

typedef struct {
    node_t *head;
    node_t *tail;
    int size;
} list_t;

void list_init (list_t* l);
int list_size (list_t l);
void list_insert_back (list_t *l, PVOID data);
void list_clear (list_t *l);
void list_print (list_t l);

#endif