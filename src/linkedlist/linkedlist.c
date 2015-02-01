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

/*
 * The five following functions handle the low-order mark bit that indicates
 * whether a node is logically deleted (1) or not (0).
 *  - is_marked_ref returns whether it is marked, 
 *  - (un)set_marked changes the mark,
 *  - get_(un)marked_ref sets the mark before returning the node.
 */
inline int
is_marked_ref(long i) 
{
  return (int) (i & 0x1L);
}

inline long
unset_mark(long i)
{
  i &= ~0x1L;
  return i;
}

inline long
set_mark(long i) 
{
  i |= 0x1L;
  return i;
}

inline long
get_unmarked_ref(long w) 
{
  return w & ~0x1L;
}

inline long
get_marked_ref(long w) 
{
  return w | 0x1L;
}

/*
 * list_search looks for value val, it
 *  - returns right_node owning val (if present) or its immediately higher 
 *    value present in the list (otherwise) and 
 *  - sets the left_node to the node owning the value immediately lower than val. 
 * Encountered nodes that are marked as logically deleted are physically removed
 * from the list, yet not garbage collected.
 */
node_t* list_search(llist_t* l, val_t val, node_t** left)
{
    node_t* right;
    node_t* i;

    repeat_search:
    do {
        // Step 1: Traverse the list and find left (<val) and right (>=val).
        i = l->head;
        do {
            if (is_marked_ref(i->next)) continue; // Skip marked nodes.
            if (i->val >= val) {
                right = i;
                break;
            }
            (*left) = i;
        } while ((i = get_unmarked_ref(i->next)));

        // Step 2: If there are marked nodes between left and right try to "remove" them.
        if ((*left)->next != right) {

            // Step 2.1: If an insertions was made in the meantime between left and right, repeat search.
            i = get_unmarked_ref((*left)->next);
            do {
                // If there is at least one unmarked, search again.
                if (!is_marked_ref(i->next)) goto repeat_search;
            } while ((i = get_unmarked_ref(i->next)) != right);
            // No insertions were made at this point!

            // Step 2.2: Try to "remove" the marked nodes between left and right.
            if (!CAS_PTR_BOOL(&((*left)->next), get_unmarked_ref((*left)->next), right))
                goto repeat_search; // Search again if somone changed left->next.
        }

        // At this point, left->next == right. Safe to return!
        return right;
    } while (1);
}

// Returns 1 if found, 0 otherwise.
int list_contains(llist_t* l, val_t val)
{
    node_t* left;
    return (list_search(l, val, &left)->val == val);
}

// *Unused*
node_t* new_node(val_t val, node_t* next)
{
    node_t* n = malloc(sizeof(node_t));
    n->val = val;
    n->next = next;
    return n;
}

// Returns a new list and initializes the memory pool.
llist_t* list_new()
{
    llist_t* l = (llist_t*) malloc(sizeof(llist_t));
    
    // Initialize tail.
    l->tail = malloc(sizeof(node_t));
    l->tail->val = INT_MAX;
    l->tail->next = NULL;
    
    // Initialize head.
    l->head = malloc(sizeof(node_t));
    l->head->val = INT_MIN;
    l->head->next = l->tail;

    // Initialize the memory pool and the first block.
    memptr = 0;
    mem = malloc(MEM_BLOCK_CNT*sizeof(node_t));
    mem[0] = malloc(MEM_BLOCK_SIZE*sizeof(node_t));

    return l;
}

// Deletes the entire memory pool and the entire list.
void list_delete(llist_t* l)
{
    int i = 0;
    while(mem[i] != NULL) {
        free(mem[i]);
        i += 1;
    }
    free(mem);
    free(l->head);
    free(l->tail);	
    free(l);
}

// Traverses the list and increments size counter.
int list_size(llist_t* l)
{
    int size = 0; // without head + tail.
    node_t* i = l->head->next;
    while (i != l->tail) {
        size += 1;
        i = get_unmarked_ref(i->next);
    }
    return size;
}

// Inserts a new node with value val in the list and returns 1,
// or returns 0 if a node with that value already exists.
int list_add(llist_t* l, val_t val)
{
    node_t* n = NULL;
    node_t* left;
    node_t* right;

    do {
        // Search for left and right nodes.
        right = list_search(l, val, &left);
        if (right->val == val) return 0; // already exists.
        
        // n does not exist! Initialize it and insert it.
        if (n == NULL) {
            // Fetch and increment the global memory pointer.
	        uint32_t my_memptr = FAI_U32(&memptr);
            // Figure out what block to use.
	        uint32_t my_memblock = my_memptr/MEM_BLOCK_SIZE;

            // If that block is a new one, initialize it.
	        if(mem[my_memblock] == NULL) {
	            node_t* tmpmem = malloc(MEM_BLOCK_SIZE*sizeof(node_t));
                // Only one succeeds to initialize it. The rest free the temporary malloc.
	            if(!CAS_PTR_BOOL(&mem[my_memblock], NULL, tmpmem)) free(tmpmem);
	        }
            // Assign n a place in memory.
	        n = &mem[my_memblock][my_memptr%MEM_BLOCK_SIZE];
		    n->val = val;
        }
        n->next = right; // point to right.

        // try to change left->next to point to n instead of right.
        if (CAS_PTR_BOOL(&(left->next), right, n)) return 1;
        // If CAS fails, someone messed up. Retry!
    } while (1);
}

// Removes the node with value val from the list and returns 1,
// or returns 0 if the node with that value did not exists.
// The node is "removed" by marking its "next" field.
int list_remove(llist_t* l, val_t val)
{
    node_t* left;
    node_t* right;
    
    do {
        // Search for left and right nodes.
        right = list_search(l, val, &left);
        if (right->val != val) return 0; // does not exist.
        
        // n exists! Try to mark right->next.
    	if (CAS_PTR_BOOL(&(right->next), get_unmarked_ref(right->next), get_marked_ref(right->next))) {
            // Also try to link left with right->next. if it fails it's ok - someone else fixed it.
            CAS_PTR(&(left->next), right, get_unmarked_ref(right->next));
            return 1;
    	}
        // If CAS fails, something changed. Retry!
    } while (1);
}

