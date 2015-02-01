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
//#include "lock_if.h" Implemented here
#include "utils.h"

#ifdef DEBUG
#define IO_FLUSH                        fflush(NULL)
#endif

typedef intptr_t val_t;

// boolean CAS_PTR for ease of coding.
#define CAS_PTR_BOOL(addr, old, new) (old == CAS_PTR(addr, old, new))

typedef struct node {
    val_t val;
    struct node *next;
    // <lock>
    volatile uint32_t lock_head;
    volatile uint32_t lock_tail;
    // </lock>
} node_t;

typedef struct llist {
    node_t *head; // INT_MIN
    node_t *tail; // INT_MAX
} llist_t;

// Absolute value of substraction.
static inline uint32_t absval(const uint32_t l, const uint32_t r) {
    return (l > r) ? (l - r) : (r - l);
}

// Locks a node using its ticket lock.
static inline void lock_lock(volatile node_t** l) {
    uint32_t my_ticket = IAF_U32(&((*l)->lock_tail)); // inc n fetch my ticket.
    do {
        uint32_t dist = absval(my_ticket, (*l)->lock_head);
        if (dist == 0) return; // If dist==0 return.
        nop_rep(dist * 100); // Else do nothing for dist*100.
    } while (1);
    asm volatile("" ::: "memory");
}

// Unlocks a node's lock by incrementing the head.
static inline void lock_unlock(volatile node_t** l) {
    asm volatile("" ::: "memory");
    (*l)->lock_head += 1;
}

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

#endif

