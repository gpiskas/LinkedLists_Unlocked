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


#ifndef _LOCK_IF_H_
#define _LOCK_IF_H_

#include "utils.h"

#if defined(LOCKTYPE)
typedef struct tlock
{
    volatile uint32_t head;
    volatile uint32_t tail;
} tlock_t;

#  define INIT_LOCK(lock)				lock_init((lock)
#  define DESTROY_LOCK(lock)			lock_destroy(lock)
#  define LOCK(lock)					lock_lock(lock)
#  define UNLOCK(lock)					lock_unlock(lock)

static inline void
lock_init(volatile tlock_t** l)
{
    *l = (tlock_t*) malloc(sizeof(tlock_t));
    (*l)->head = 1;
    (*l)->tail = 0;
    asm volatile("" ::: "memory");
}

static inline void
lock_destroy(volatile tlock_t* l)
{
    free(l);
}

static inline uint32_t absval(const uint32_t l, const uint32_t r) {
    return (l > r) ? (l - r) : (r - l);
}

static inline void
lock_lock(volatile tlock_t* l)
{
    uint32_t my_ticket = IAF_U32(&(l->tail)); // inc n fetch my ticket.
    do {
        uint32_t dist = absval(my_ticket, l->head);
        if (dist == 0) return; // If dist==0 return.
        nop_rep(dist * 100); // Else do nothing for dist*100.
    } while (1);
    asm volatile("" ::: "memory");
}

static inline void
lock_unlock(volatile tlock_t* l)
{
    asm volatile("" ::: "memory");
    l->head += 1;
}
#endif

#endif	/* _LOCK_IF_H_ */
