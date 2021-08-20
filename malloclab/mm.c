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
 * AN: Discussion of general virtual memory model
 *
 * Provided with the lab is the memlib module which models a virtual
 * memory of MAX_HEAP ~20MB, allocated by the actual libc malloc service. 
 * The mdriver initializes this VM model by calling the mem_init() from 
 * the provided module, and drives all the testing with its main routine.
 * 
 * The memlib module maintains the start of the heap in the model, the 
 * current boundary of the heap in use (mem_brk) and the maximum legal 
 * size of the heap, mem_max_addr. mem_brk is updated via the mem_sbrk()
 * function when the heap is initialized, and subsequently whenever it
 * is extended, should mm_malloc not find a suitable block size.
 *
 * The student implementation (below) is initialized via mm_init. The 
 * core implementations of mm_malloc, mm_free, and mm_realloc are then
 * called by the mdriver on the traces provided with this lab.
 *
 *
 * AN: Description of implementation #1: implicit free list
 *
 * The allocator models the heap as an in-place list; at an arbitrary
 * moment of program execution, the list has an 8 byte prologue block with 
 * only a header+footer, followed by a number of varying-size regular blocks
 * each with a header and footer, and ended by a 0-byte epilogue block 
 * consisting only of a header.
 *
 * word = 4bytes; double = 8bytes; HDR=FTR=word; min blk size=4words=16bytes.
 * 
 * When traversing the list, the next block is implicitly determined by
 * adding the current block's size to its ptr. Allocation is possible for
 * consecutive locations of the heap; a block's neighbours in the list are
 * also the ones adjacent to it in memory. The list can thus be at any point
 * a mix of free and allocated blocks. 
 *
 * ADDON_0: inserted necessary code for next_fit() into mm_init and coalesce:
 * make sure that the rover which remembers where the fitment fcn left off
 * is not pointing into a block which just got coalesced. Without this
 * verification, the rover will eventually point into a coalesced block,
 * and given the right allocation bit value, the next payload will overlap
 * with an existing one.
 *
 * ADDON_1: modification to eliminate need for a footer in allocated blox.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/* macros for memory management. */
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
static void *rover;

/* declare helper fcns */
static void *extend_heap (size_t words);
static void *coalesce(void *bp);
static void defragment(void);
static void *first_fit(size_t asize);
static void *best_fit(size_t asize);
static void *next_fit(size_t asize);
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

	/* alignment padding; prologue hdr; prologue ftr; epilogue hdr */
	PUT(heap_listp, 0);
	PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1+2));
	PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1+2));
	PUT(heap_listp + (3*WSIZE), PACK(0, 1+2));
	heap_listp+=(2*WSIZE); // points right after prologue block?

	/* ADDON_0: initial next fit search starts at beginning */
	rover = heap_listp;

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
    	/* ADDON_1: in case of no-footer design, 4 for overhead (potential error)*/
    	asize = ((size + WSIZE + (DSIZE - 1))/DSIZE) * DSIZE;
    }

    /* search for suitable block via some fit method, and allocate */
    if ((bp = first_fit(asize)) != NULL) {
    	place(bp, asize);
    	// mm_check();
    	return bp;
    }


    /* if no block large enough, extend heap, place+coalesce if possible, return bp */
    extendedsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendedsize/WSIZE)) == NULL) 
    	return NULL;

    // mm_check();
    place(bp, extendedsize);
    return bp;


}

/*
 * mm_free - 
 * naive: 	Freeing a block does nothing. 
 * implicit list: marks block as free, coalesces with free neighbours.
 *
 */
void mm_free(void *ptr)
{
	
	/* First implementation of free */

    /* mark hdr and ftr as free */
    size_t size = GET_SIZE(HDRP(ptr));
    int prev_alloc;

    /* ADDON_1: get status of previous block */
    prev_alloc = GET_ALLOC_PREV(HDRP(ptr));

	PUT(HDRP(ptr), PACK(size, 0+prev_alloc));
	PUT(FTRP(ptr), PACK(size, 0+prev_alloc));

	/* ADDON_1: inform next block that the current one is free */
	PUT(HDRP(NEXT_BLKP(ptr)), PACK(GET_SIZE(HDRP(NEXT_BLKP(ptr))), GET_ALLOC(HDRP(NEXT_BLKP(ptr)))));

	coalesce(ptr);

}

/*
 * mm_realloc - 
 * basic: 	Implemented simply in terms of mm_malloc and mm_free
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

}






/* --------------- helper functions --------------- */


/* extend the heap with a new free block.
 * called when initializing the allocator, and when no block found.
 * NOTE: calling with small chunks might make debugging difficult
 */
static void *extend_heap (size_t words)
{
	size_t size;
	char *bp;
	/* ADDON_1: status of block before epilogue */
	int prev_alloc;

	/* allocate even nr of words */
	size = (words % 2)? ((words+1) * WSIZE) : (words * WSIZE);
	if ((long)(bp = mem_sbrk(size)) == -1)
		return NULL;

	/* ADDON_1: extract epilogue information before extension inserted */
	prev_alloc = GET_ALLOC_PREV(HDRP(NEXT_BLKP(bp)));

	/* prep the header, footer and epilogue header */
	PUT(HDRP(bp), PACK(size, 0+prev_alloc));
	PUT(FTRP(bp), PACK(size, 0+prev_alloc));
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // epilogue's prev. not allocated

	// mm_check();

	return coalesce(bp); /* coalesce if prev block free */

}

/* Coalesce with neighboring blocks if they are free. 
 * * this fcn assumes that the current block, bp, is free,
 * * and that it has info on previous block alloc status
 */
static void *coalesce(void *bp) 
{
	/* status of neighbor blocks, size of current */
	/* ADDON_1: previous alloc status should always be checked at current block hdr */
	// size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t prev_alloc = GET_ALLOC_PREV(HDRP(bp)) / 2; // ==(2 / 2) if allocated
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));
	int prev_prev_alloc;

	/* possible cases when coalescing with neighbors */

	if (prev_alloc & next_alloc) { /* both allocated */
		// mm_check();

		return bp;

	} else if (prev_alloc & (!next_alloc)) { /* coalesce with next*/
		size+= GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0+2));
		PUT(FTRP(bp), PACK(size, 0+2)); 

	} else if ((!prev_alloc) & next_alloc) { /* coalesce with prev */
		/* ADDON_1: obtain info on the second to previous block */
		prev_prev_alloc = GET_ALLOC_PREV(HDRP(PREV_BLKP(bp)));
		size+= GET_SIZE(FTRP(PREV_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, prev_prev_alloc+0));
		PUT(FTRP(bp), PACK(size, prev_prev_alloc+0));
		bp = PREV_BLKP(bp);

	} else { /* coalesce with both */
		/* ADDON_1: obtain info on the second to previous block */
		prev_prev_alloc = GET_ALLOC_PREV(HDRP(PREV_BLKP(bp)));
		size+= GET_SIZE(FTRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, prev_prev_alloc+0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, prev_prev_alloc+0));
		bp = PREV_BLKP(bp);
	}


	/* ADDON_0: This code needed for next_fit() */
	if ((rover > bp) && (rover < NEXT_BLKP(bp)))
		rover = (bp);


	return bp;
}


/* Fitment functions */

/* first fit */
static void *first_fit(size_t asize) 
{

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

/* best fit; bad performance with implicit list; entire list must be scanned */ 
static void *best_fit(size_t asize) 
{
	return NULL;
}

/* next fit: performance almost identical to first_fit in implicit list.
 * start each search where the last stopped (rover), onwards till it wraps 
 * back to the beginning of the implicit list, and up to the rover. */
static void *next_fit(size_t asize) 
{

	void *oldrover = rover;
	
	/* from rover to end of block list */
	while (GET_SIZE(HDRP(rover))) {
	
		if (!GET_ALLOC(HDRP(rover)) && (asize <= GET_SIZE(HDRP(rover)))) {
			return rover;
		}
		
		rover = NEXT_BLKP(rover);
	}


	/* from start of block list to rover */
	rover = (heap_listp);
	while (/*GET_SIZE(HDRP(rover)) &&*/ (rover < oldrover)) {

		if (!GET_ALLOC(HDRP(rover)) && (asize <= GET_SIZE(HDRP(rover)))) {
			return rover;
		}
		
		rover = NEXT_BLKP(rover);
	}

	/* if not found */
	return NULL;

}



/* placement helper 
 * assumes that bp is free, and the block big enough for asize
 * after this call, bp points to an allocated block.
 * it may produce an unallocated next block.
 */
static void place(void *bp, size_t asize) {

	size_t remainder = GET_SIZE(HDRP(bp)) - asize;
	size_t minimum_split = 2*DSIZE; // remainder

	if ( remainder >= (minimum_split) ) { // split
		
		/* shorten the allocated block; */
		/* ADDON_1: get status of previous and put that in too */
		PUT(HDRP(bp), PACK(asize, GET_ALLOC_PREV(HDRP(bp)) + 1));
		/* ADDON_1: don't include ftr */
		// PUT(FTRP(bp), PACK(asize, 1)); 
		
		/* cut remainder to leave a free block */
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(remainder, 0+2));
		PUT(FTRP(bp), PACK(remainder, 0+2));
		
	} else { // keep current block size
		/* store status of current block */
		/* ADDON_1: get status of previous as well */
		PUT(HDRP(bp), PACK(GET_SIZE(HDRP(bp)), GET_ALLOC_PREV(HDRP(bp)) + 1));
		// PUT(FTRP(bp), PACK(GET_SIZE(HDRP(bp)), 1)); 

		/* ADDON_1: inform next block that current is allocated */
		PUT(HDRP(NEXT_BLKP(bp)), PACK(GET_SIZE(HDRP(NEXT_BLKP(bp))), GET_ALLOC(HDRP(NEXT_BLKP(bp))) + 2));
	
	}
	return;
}

/* a defrag routine, called at every free.
 * coalesces if a block is unallocated
 */
static void defragment(void)
{

	void *bp = heap_listp;

	while (GET_SIZE(HDRP(bp))) {

		if (!(GET_ALLOC(HDRP(bp))))
			bp = coalesce(bp);
		
		bp = NEXT_BLKP(bp);
	}


}

/* heap checker - where to call, what to report? 
 *
 * is every block in free list marked as free?
 * any contiguous free blocks that escaped coalescing?
 * is every free block actually in the free list?
 * do the pointers in the free list point to free blocks?
 * do the pointers in a heap block point to valid heap addresses?

 * do any allocated blocks overlap? mdriver tracks blocks via linked list,
 * where each node is constructed from a line in a trace file. given a 
 * known heap start, it can also track payloads and check if they overlap, 
 * as the driver calls malloc and scans the LL struct nodes

 */
static int mm_check(void)
{
	
	/* any contiguous free blocks that escaped coalescing? */
	int prev_allocated = 1;
	int unmerged_free_blocks = 0; // counts each border bracketed by free blox
	void *bp = heap_listp;


	while (GET_SIZE(HDRP(bp))) {

		if (!(GET_ALLOC(HDRP(bp))) & !(prev_allocated)) {
			unmerged_free_blocks++;
			
		}

		prev_allocated = GET_ALLOC(HDRP(bp));
		
		bp = NEXT_BLKP(bp);
	}

	printf("There are %d unmerged free blocks. \n", unmerged_free_blocks);


	return 1; // HEAP OK

}





