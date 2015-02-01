/*
 * Lock-free & Lock-based Linked List implementation.
 * Copyright (C) 2015 George Piskas
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Contact: geopiskas@gmail.com
 */

#ifndef LLIST_H_ 
#define LLIST_H_


#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>

#include "atomic_ops_if.h"

#ifdef DEBUG
#define IO_FLUSH                        fflush(NULL)
#endif

typedef intptr_t val_t;

// boolean CAS_PTR for ease of coding.
#define CAS_PTR_BOOL(addr, old, new) (old == CAS_PTR(addr, old, new))

typedef struct node {
    val_t val;
    struct node *next;
} node_t;


typedef struct llist {
    node_t *head; // INT_MIN
    node_t *tail; // INT_MAX
} llist_t;

// Memory pool
node_t **mem; // Memory blocks
uint32_t memptr; // Current cell
#define MEM_BLOCK_SIZE 1000000 //16MB (node_t = 16b)
#define MEM_BLOCK_CNT 500 // 8GB of mem max

inline int is_marked_ref(long i);
inline long unset_mark(long i);
inline long set_mark(long i);
inline long get_unmarked_ref(long w);
inline long get_marked_ref(long w);


llist_t* list_new();
//return 0 if not found, positive number otherwise
int list_contains(llist_t *the_list, val_t val);
//return 0 if value already in the list, positive number otherwise
int list_add(llist_t *the_list, val_t val);
//return 0 if value already in the list, positive number otherwise
int list_remove(llist_t *the_list, val_t val);
void list_delete(llist_t *the_list);
int list_size(llist_t *the_list);


node_t* new_node(val_t val, node_t* next);
node_t* list_search(llist_t* the_list, val_t val, node_t** left_node);


#endif
