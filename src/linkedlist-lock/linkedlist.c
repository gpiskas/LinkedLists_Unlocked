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


#include "linkedlist.h"


// Returns 1 if found, 0 otherwise.
int list_contains(llist_t* l, val_t val)
{
    // Traverse the list until we reach or exceed val.
    node_t* left = l->head->next;
    while (left->val < val) left = left->next;
    
    // Return 1 if found, else 0.
    if (left->val == val) return 1;
    return 0;
}

// *Unused*
node_t* new_node(val_t val, node_t* next)
{
    node_t* n = (node_t*) malloc(sizeof(node_t));
    n->val = val;
    n->next = next;
    return n;
}


// Returns a newly initialized list.
llist_t* list_new()
{
    llist_t* l = (llist_t*) malloc(sizeof(llist_t));
    
    // Initialize tail.
    l->tail = malloc(sizeof(node_t));
    l->tail->val = INT_MAX;
    l->tail->next = NULL;
    // Initialize lock.
    l->tail->lock_head = 1;
    l->tail->lock_tail = 0;
    
    // Initialize head.
    l->head = malloc(sizeof(node_t));
    l->head->val = INT_MIN;
    l->head->next = l->tail;
    // Initialize lock.
    l->head->lock_head = 1;
    l->head->lock_tail = 0;

    return l;
}

// Deletes the entire list.
void list_delete(llist_t* l)
{
    node_t* i = l->head;
    node_t* del = i;
    do {
        free(del);
        del = i;
    } while (i);
    free(l);
}

// Traverses the list and increments size counter.
int list_size(llist_t* l)
{
    int size = 0; // without head + tail.
    node_t* i = l->head->next;
    while (i != l->tail) {
        size += 1;
        i = i->next;
    }
    return size;
}

// Inserts a new node with value val in the list and returns 1,
// or returns 0 if a node with that value already exists.
int list_add(llist_t* l, val_t val)
{
    // Lock the head of the list and begin.
    // We only need to lock the predecessor to add a new node.
    node_t* left = l->head;
    lock_lock(&left);
    
    // Traverse the list until we reach or exceed val.
    // Keep the element on the left of where val should be always locked.
    while (left->next->val < val) {
        lock_lock(&left->next); // Lock next.
        lock_unlock(&left); // Unlock prevous.
        left = left->next; // Move to the next.
    }

    // If a node with value val already exists, return 0.
    if(left->next->val == val) {
        lock_unlock(&left); // Always unlock!
        return 0;
    }
    
    // n does not exist! Initialize it...
    node_t* n = malloc(sizeof(node_t));
    n->val = val;
    n->next = left->next;
    // Initialize lock.
    n->lock_head = 1;
    n->lock_tail = 0;
    
    // ...and insert it.
    left->next = n;
    lock_unlock(&left); // Always unlock!
    return 1;
}

// Removes the node with value val from the list and returns 1,
// or returns 0 if the node with that value did not exists.
int list_remove(llist_t* l, val_t val)
{
    // Lock the head and its next node and begin.
    // We need to lock both the predecessor and the current (right) node to remove it.
    node_t* left = l->head;
    lock_lock(&left);
    node_t* right = left->next;
    lock_lock(&right);
    
    // Traverse the list until we reach or exceed val.
    // Proceed using the hand-over-hand technique.
    while (right->val < val) {
        lock_unlock(&left); // Unlock predecessor.
        left = right; // Move predecessor to current.
        right = right->next; // Move current the next.
        lock_lock(&right); // Lock current.
    }
    
    // If a node with value val does not exist, return 0.
    if(right->val != val) {
        lock_unlock(&right); // Always unlock!
        lock_unlock(&left); // Always unlock!
        return 0;
    }

    left->next = right->next;
    lock_unlock(&right); // Always unlock!
    lock_unlock(&left); // Always unlock!
    return 1;
}

