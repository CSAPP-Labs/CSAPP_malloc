#include "mm_macros.c"

/* global variables for allocator design */
static void *heap_listp; /* always points at prologue block of heap */
static void *list_start; /* for explicit free list, linked list design */

/* payload_size could be included in the header for overlap, and to track internal fragmentation*/
static int minblock = 3*DSIZE; /* hdr + pred + succ + ftr (+ request_id + payload_size, for debugging) */

/* global variables for debugging*/
static int request_count = -1;
/* depends on trace file; this limit is to prematurely end mdriver tests and inspect 
 * where the allocation might have failed. if the entire trace should be run, then
 * the limit is ten times that of its requests, because mdriver runs each trace x10*/
static int max_requests = 6000 * 10; 
static int verbose = 0;


/* declare heap checker helpers */
static int mm_check(int verbose);
static int checkheap(int verbose);
static int checklist(int verbose);
static void printblock(void *bp);
static int checkblock(void *bp);
static void printnode(void *bp);
static int checknode(void *bp);
static int blockNotInList(void *ptr);
