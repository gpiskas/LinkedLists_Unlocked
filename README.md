Lock-free & Lock-based Linked List implementation. 
==========

References:
* A thorough explanation of the algorithms we will implement:
http://www.cs.nyu.edu/courses/fall05/G22.2631-001/lists.slides2.pdf
* Lock-free linkedlist implementation of Harris' algorithm
"A Pragmatic Implementation of Non-Blocking Linked Lists"
T. Harris, p. 300-314, DISC 2001.

Notes:
* The lock-based version uses a ticket lock with backoff.
* The lock-free version uses a custom memory pool for allocations.
* Garbage collection is not implemented as maximum performance was goal purpose of this project.
* Uses Compare & Swap.
* Run make in the base dir.

Lock-free & Lock-based Linked List implementation.  (C) 2015 George Piskas
