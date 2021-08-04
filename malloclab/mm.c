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
 * AN: General discussion
 *
 * Provided with the lab is the memlib module which models a virtual
 * memory of MAX_HEAP ~20MB, allocated by the actual libc malloc service. 
 * The mdriver initializes this VM model by calling the mem_init() from 
 * the provided module, and drives all the testing with its main routine.
 *
 * The student implementation (below) is initialized via mm_init. The 
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

/* macros for memory management. to be included in makefile? */
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

/* declare helper fcns */
static void *extend_heap (size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static int mm_check(void);



/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{

	/* first implementation */
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

	/* basic implementation of malloc */
	/*
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
	*/

    /* First implementation of malloc */

    /* pad the requested size, return if size too small */
    size_t asize;
    size_t extendedsize;
    char *bp;

    if (size == 0)
    	return NULL;

    if (size <= DSIZE) {
    	asize = 2*DSIZE;
    } else {
    	/* round up to nearest 8, and add 8 for overhead */
    	asize = ((size + DSIZE + DSIZE - 1)/DSIZE) * DSIZE;
    }

    /* search for suitable block via some fit method, and allocate */
    if ((bp = find_fit(asize)) != NULL) {
    	place(bp, asize);
    	return bp;
    }


    /* if no block large enough, extend heap, place+coalesce if possible, return bp */
    extendedsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendedsize/WSIZE)) == NULL) 
    	return NULL;

    place(bp, extendedsize);
    return bp;


}

/*
 * mm_free - Freeing a block does nothing. (in basic version)
 */
void mm_free(void *ptr)
{

	/* First implementation of free */
    /* mark hdr and ftr as free, and coalesce with neighbors if possible */
    size_t size = GET_SIZE(HDRP(ptr));
	PUT(HDRP(ptr), PACK(size, 0));
	PUT(FTRP(ptr), PACK(size, 0));
	coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{

	/* basic implementation of realloc */
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

    /* first implementation of realloc */


}


/* helper functions */

/* heap checker - where to call, what to report? 
 *
 * is every block in free list marked as free?
 * any contiguous free blocks that escaped coalescing?
 * is every free block actually in the free list?
 * do the pointers in the free list point to free blocks?
 * do any allocated blocks overlap?
 * do the pointers in a heap block point to valid heap addresses?
 */
static int mm_check(void)
{



	return 1; // HEAP OK

}


/* extend the heap with a new free block.
 * called upon initializing the allocator, and whenever no block found.
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

/* coalesce with neighboring blocks if they are free */
static void *coalesce(void *bp) 
{
	/* status of neighbor blocks, size of current */
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));

	/* possible cases when coalescing with neighbors */

	if (prev_alloc & next_alloc) { /* both allocated */
		return bp;

	} else if (prev_alloc & (!next_alloc)) { /* coalesce with next*/
		size+= GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0)); 	/* because current hdr has new size 
										 * as updated by previous line
										 */

	} else if ((!prev_alloc) & next_alloc) { /* coalesce with prev */
		size+= GET_SIZE(FTRP(PREV_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
		bp = PREV_BLKP(bp);

	} else { /* coalesce with both */
		size+= GET_SIZE(FTRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}


	return bp;
}

/* first fit */
static void *find_fit(size_t asize) {

	void *bp = heap_listp; 
	
	// till we get to epilogue whose size is 0
	while (GET_SIZE(HDRP(bp))) {
	
		if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
			return bp;
		
		bp = NEXT_BLKP(bp);
	}

	// if not found
	return NULL;

}

/* placement helper */
static void place(void *bp, size_t asize) {

	size_t remainder = GET_SIZE(HDRP(bp)) - asize;
	size_t rounded_remainder;

	if ( remainder >= (2*DSIZE) ) { // split
		
		// shorten the allocated block
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1)); // ftr pos based on hdr content?
		
		// create the cut, free block
		bp = NEXT_BLKP(bp);
		rounded_remainder = DSIZE * ((remainder + DSIZE - 1)/DSIZE);
		PUT(HDRP(bp), PACK(rounded_remainder, 0));
		PUT(FTRP(bp), PACK(rounded_remainder, 0));
		
	} else { // keep current block size
		PUT(HDRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
		PUT(FTRP(bp), PACK(GET_SIZE(HDRP(bp)), 1)); 
	
	}
	return;
}







