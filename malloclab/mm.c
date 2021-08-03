/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 * 
 *
 * (AN): General discussion
 *
 * Provided with the lab is the memlib module which models a virtual
 * memory of MAX_HEAP ~20MB, allocated by the actual libc malloc service. 
 * The mdriver initializes this VM model by calling the mem_init() from 
 * the provided module. 
 * Within this model,
 * the student implementation (below) is initialized via mm_init. The 
 * core implementations of mm_malloc, mm_free, and mm_realloc are then
 * called by the mdriver on the traces provided with this lab.
 *
 * 
 * The memlib module maintains the start of the heap in the model, the 
 * current boundary of the heap in use (mem_brk) and the maximum legal 
 * size of the heap, mem_max_addr. mem_brk is updated via the mem_sbrk()
 * function when the heap is initialized, and subsequently whenever it
 * is extended, should mm_malloc not find a suitable block size.
 * 
 * The core student implementations use macros from mm_macros.c. 
 * mm_init() initializes the heap with CHUNKSIZE.  
 *
 *
 *
 * AN: Description of implementation
 * This implementation models the heap as an implicit free list, and 
 * blocks with headers and footers.

 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"
// macros for memory management. to be included in makefile?
#include "mm_macros.c"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "anteam",
    /* First member's full name */
    "Adam Naas",
    /* First member's email address */
    "not applicable",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* always points at prologue block of heap */
static void *heap_listp; // = mem_heap_lo();


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
	/* initial empty heap */
	if ( (heap_listp = mem_sbrk(2*DSIZE))== (void *)-1)
		return -1;

	/* alignment; prologue hdr; prologue ftr; epilogue hdr */
	PUT(heap_listp, 0);
	PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));
	PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));
	PUT(heap_listp + (3*WSIZE), PACK(0, 1));
	heap_listp+=DSIZE; // points right after prologue block?


	/* extend heap with a free block of CHUNKSIZE */
	if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
		return -1;

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}


/* 
 * helper functions: placement, find fits, heap checker
 *
 */

/* heap checker - where to call, what to report? 
 *
 * is every block in free list marked as free?
 * any contiguous free blocks that escaped coalescing?
 * is every free block actually in the free list?
 * do the pointers in the free list point to free blocks?
 * do any allocated blocks overlap?
 * do the pointers in a heap block point to valid heap addresses?
 */
int mm_check(void)
{



	return 1; // HEAP OK

}


/* extend the heap with a new free block. reconfigure the epilogue
 * hdr. coalesce with adjacent block if the latter is free.
 *
 * called upon initializing the allocator, and whenever no block found.
 *
 */
static void *extend_heap (size_t words)
{
	size_t size;
	char *bp;

	/* allocate even nr of words */
	size = (words % 2)? ((words+1) * WSIZE) : (words * WSIZE);
	if ((long)(bp = mem_sbrk(size)) == -1)
		return NULL;

	/* prep the header, footer and epilogue header */
	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

	return coalesce(bp); /* coalesce if prev block free */

}

static void *coalesce(void *bp) 
{



	
}









