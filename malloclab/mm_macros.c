/*
 * macro helpers for mm.c
 */

/* basic size definitions */
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<8)

#define MAX(x, y) ( ((x) > (y)) ? (x) : (y) )


/* helper operations, getters, setters 
 * p = general ptr
 * bp = block ptr 
 * char casting is to avoid scaling when doing ptr arithmetic
 */
#define PACK(size, alloc) ((size) | (alloc))
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val)) /* stores 4 bytes at increasing address? */

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char *)bp - WSIZE)
#define FTRP(bp) ((char *)bp + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char *)bp + GET_SIZE((char *)bp - WSIZE))  /* can also be done by (char *)FTRP(bp) + DSIZE ?*/
#define PREV_BLKP(bp) ((char *)bp - GET_SIZE((char *)bp - DSIZE))  /* operates on previous footer */

/* ADDON_1: for optimizing the footer out of allocated blocks 
 * Status of current block is in the lowest bit, status of previous
 * is in the next bit.
 */
#define GET_ALLOC_PREV(p) (GET(p) & 0x2)

/* Explicit free list: linked list traversal helpers */

/* this should be doable with the PUT macro */
#define PRED(bp) ((unsigned int **)((char *)bp))
#define SUCC(bp) ((unsigned int **)((char *)bp + DSIZE))

/* debugging helpers */
// #define DEBUG

#ifndef DEBUG
#define CHECK(msg, verbose, arg1, arg2) ((void)0) 
#define INTERRUPT(count, max) ((void)0)
#else
#define CHECK(msg, verbose, arg1, arg2) if (verbose == 1) {printf(msg, arg1, arg2);} mm_check(verbose)
#define INTERRUPT(count, max) if (count > max) {exit(0);}
#endif




