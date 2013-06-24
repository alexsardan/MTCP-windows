/****************************************************************************
 * Singly linked list implementation
 *
 * Copyright (C) 2013, Alexandru-Cezar Sardan, Marius-Alexandru Caba
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 ****************************************************************************/

#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void list_init (list_t* l)
{
    l->head = NULL;
    l->tail = NULL;
    l->size = 0;
}

int list_size (list_t l)
{
    return l.size;
}

void list_print (list_t l)
{
	int i;
	node_t *current = l.head;
	for (i = 0; i < l.size; i++)
	{
		printf ("%p\n", current->data);
		current = current->next;
	}
}

void list_insert_back (list_t *l, PVOID data)
{
	node_t *nod = (node_t *) malloc (sizeof(node_t));
    nod->next = NULL;
    nod->data = data;
    
    if (l->head == NULL)
    {
        l->head = nod;
        l->tail = nod;
    } 
    else
    {
        l->tail->next = nod;
        l->tail = nod;
    }
    l->size++;
}

void list_clear (list_t *l)
{
    node_t *next;
    if (l->head == NULL)
        return;
    else
    {
        while (l->head != NULL)
        {
            next = l->head->next;
            free (l->head);
            l->head = next;
        }
    }
    l->tail = NULL;
    l->size = 0;
}