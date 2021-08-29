/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.(initial)
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * Further comments by AN (Adam) 
 * 
 * Discussion of general virtual memory model
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
 * Design considerations
 *
 * In order to achieve an acceptable balance of allocation throughput and 
 * heap utilization, several design issues are considered: 
 * free block organization, placement of payload into free block, coalescing
 * of free blocks, and splitting of a large-enough free block.
 *
 * Free block organization:
 * The list of free blocks is organized with a block-search methodology in
 * mind. Minimum block size and alignment is enforced throughout the entire
 * allocator implementation.
 *
 *
 * Description of implementation #1: implicit free list
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
 *
 * Description of implementation #2: explicit free lists
 * 
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

// #define DEBUG 1

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* always points at prologue block of heap */
static void *heap_listp; // = mem_heap_lo();
static void *rover;
static void *list_start;

/* declare helper fcns */
static void *extend_heap (size_t words);
static void *coalesce(void *bp);
static void defragment(void);
static void *first_fit(size_t asize);
static void *best_fit(size_t asize);
static void *next_fit(size_t asize);
static void place(void *bp, size_t asize);

/* heap checker*/
static int mm_check(void);
static int checkheap(void);
static int checklist(void);
static void printblock(void *bp);
static int checkblock(void *bp);
static void printnode(void *bp);
static int checknode(void *bp);

static int request_count = -1;
static int max_requests = 1400;

/* linked list helpers */
static void prependBlockToList(void *bp, unsigned int *target);
static void removeBlockFromList(void *bp);


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{

	/* initial empty heap */
	if ( (heap_listp = mem_sbrk(4*WSIZE))== (void *)-1)
		return -1;

	/* alignment padding */
	PUT(heap_listp, 0);

	/* prologue hdr + ftr, epilogue hdr */
	PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1+2)); /*hdr*/
	PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1+2)); /*ftr*/
	PUT(heap_listp + (3*WSIZE), PACK(0, 1+2));


	/* Points at prologue, end of hdr start of ftr */
	heap_listp+=(2*WSIZE); 

	/* ADDON_0: initial next fit search starts at beginning */
	rover = heap_listp;

	/* list of free blocks starts with dummy block, otherwise NULL */
	list_start = NULL;

	/* extend heap with a free block of CHUNKSIZE, needs to be in free list */
	if ( extend_heap(CHUNKSIZE/WSIZE) == NULL)
		return -1;

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{

    /* pad the requested size, return if size too small */
    size_t asize;
    size_t extendedsize;
    char *bp;

    if (size == 0)
    	return NULL;

    request_count++;

    /* minimum block = hdr + pred + succ + ftr  = 4+8+8+4 */
    if (size <= (2*DSIZE)) {
    	asize = 3*DSIZE;
    } else {
    	/* round up to nearest 8, and add 4 for hdr; 8 if allocated block needs hdr+ftr */
    	asize = ((size + DSIZE + (DSIZE - 1))/DSIZE) * DSIZE;
    }



    INTERRUPT(request_count, max_requests);

 //    printf("Before allocation nr %d of size [%d]\n", request_count, asize);
	// mm_check();
	CHECK("Before allocation nr %d of size [%d]\n", request_count, asize);



    /* search for suitable block via some fit method, and allocate */
    /* each unique allocated bp address gets its request_id, later matched with mm_free()?*/
    if ((bp = first_fit(asize)) != NULL) {
    	place(bp, asize);

    	// printf("After allocation %d by fit fcn\n", request_count);
    	// mm_check();
    	CHECK("After allocation %d by fit fcn\n", request_count, NULL);

    	return bp;
    }

    /* defer coalescing to the case of failed allocation; similar performance */
    /*
    defragment();
     if ((bp = first_fit(asize)) != NULL) {
    	place(bp, asize);
    	return bp;
    }   
	*/

    /* if no block large enough, extend heap, place+coalesce if possible */
    extendedsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendedsize/WSIZE)) == NULL) {
    	return NULL;
    }

	// printf("After extension via extend_heap+coalesce, prior to placing.\n");
	// mm_check();
	CHECK("After extension via extend_heap+coalesce, prior to placing.\n", NULL, NULL);
    
    place(bp, asize);

	// printf("After allocation via extend_heap+coalescing, after placing.\n");
	// mm_check();
	CHECK("After allocation via extend_heap+coalescing, after placing.\n", NULL, NULL);

    return bp;


}

/*
 * mm_free for explicit linked list
 * #1: append freed block to start of list
 * #2: address-order listing
 *
 */
void mm_free(void *ptr)
{
	
    /* mark hdr and ftr as free */
    size_t size = GET_SIZE(HDRP(ptr));
    int prev_alloc;

    /* ADDON_1: get status of previous block */
    prev_alloc = GET_ALLOC_PREV(HDRP(ptr));

	PUT(HDRP(ptr), PACK(size, 0+prev_alloc));
	PUT(FTRP(ptr), PACK(size, 0+prev_alloc));

	/* ADDON_1: inform next block that the current one is free */
	PUT(HDRP(NEXT_BLKP(ptr)), PACK(GET_SIZE(HDRP(NEXT_BLKP(ptr))), GET_ALLOC(HDRP(NEXT_BLKP(ptr)))));

	/* coalescing does list insertion and pred/succ ptr setup, otherwise it would need to be done here */
	coalesce(ptr);

	// printf("After freeing addr %p\n", ptr);
	// mm_check();

	CHECK("After freeing addr %p\n", ptr, NULL);


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
	/* mem_sbrk returns the pointer which points right after the 
	 * former epilogue. */
	
	size = (words % 2)? ((words+1) * WSIZE) : (words * WSIZE);
	if ((long)(bp = mem_sbrk(size)) == -1)
		return NULL;


	/* ADDON_1: extract epilogue information before extension inserted */
	prev_alloc = GET_ALLOC_PREV(HDRP(NEXT_BLKP(bp)));

	/* prep the header, footer and epilogue header */
	/* as the heap grows, the former epilogue block gets overwritten
	 * by the header of the new block. the next epilogue is appended
	 * to the end of the current block "array". does a former epilogue
	 * ever get mistakenly accessed? */
	PUT(HDRP(bp), PACK(size, 0+prev_alloc));
	PUT(FTRP(bp), PACK(size, 0+prev_alloc));
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* epilogue's prev. not allocated */
	

	// prependBlockToList(bp, list_start);
	/* set up pointers in new free block; place it into linked list, 
	 * unless coalesce does it  */

	return coalesce(bp); /* coalesce if prev block free */

}

/* Coalesce with neighboring blocks if they are free. 
 * * this fcn assumes that the current block, bp, is free,
 * * and that it has info on previous block alloc status
 * * It is also the workhorse which adds blocks to the 
 * * free list, and patches it when removing blocks.
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
	unsigned int *target;

	/* possible cases when coalescing with neighbors */
	if (prev_alloc & next_alloc) { /* both allocated */
		/* forgot: newly freed block without free neighbours to be added to LL */
		// prependBlockToList(bp);	
		prependBlockToList(bp, list_start);
		return bp;

	} else if (prev_alloc & (!next_alloc)) { /* coalesce with next*/
		size+= GET_SIZE(HDRP(NEXT_BLKP(bp)));

		if ( NEXT_BLKP(bp) == list_start ) {
			// save future target into variable here
			target = *SUCC(list_start);
		} else {
			target = (unsigned int *)list_start;
		}

		removeBlockFromList(NEXT_BLKP(bp));

		PUT(HDRP(bp), PACK(size, 0+2));
		PUT(FTRP(bp), PACK(size, 0+2)); 

	} else if ((!prev_alloc) & next_alloc) { /* coalesce with prev */
		/* ADDON_1: obtain info on the second to previous block. Necessarily allocated? */
		prev_prev_alloc = GET_ALLOC_PREV(HDRP(PREV_BLKP(bp)));


		if ( PREV_BLKP(bp) == list_start ) {
			// store target into variable
			target = *SUCC(list_start);
		} else {
			target = (unsigned int *)list_start;
		}


		/* if removal shifts what list_start is, can cause confusion downstream*/
		/* solution to this might be saving list_start prior to removal */
		removeBlockFromList(PREV_BLKP(bp));

		size+= GET_SIZE(FTRP(PREV_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, prev_prev_alloc+0));
		PUT(FTRP(bp), PACK(size, prev_prev_alloc+0));
		bp = PREV_BLKP(bp);


	} else { /* coalesce with both */
		/* ADDON_1: obtain info on the second to previous block. Necessarily allocated? */
		prev_prev_alloc = GET_ALLOC_PREV(HDRP(PREV_BLKP(bp)));

		/* maybe target to be saved before removal */
		if ( (PREV_BLKP(bp) == list_start)) {
			removeBlockFromList(NEXT_BLKP(bp));
			// prev's succ already valid
			target = *SUCC(list_start);

		} else if ( (NEXT_BLKP(bp) == list_start) ) {
			removeBlockFromList(PREV_BLKP(bp));
			target = *SUCC(list_start);

		} else {
			removeBlockFromList(PREV_BLKP(bp));
			removeBlockFromList(NEXT_BLKP(bp));
			target = (unsigned int *)list_start;
		}


		size+= GET_SIZE(FTRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, prev_prev_alloc+0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, prev_prev_alloc+0));
		bp = PREV_BLKP(bp);

	}

	/* set up pointers in new free block; place it into linked list */
	prependBlockToList(bp, target);


	/* ADDON_0: This code needed for next_fit() */
	if ((rover > bp) && (rover < (void *)(NEXT_BLKP(bp))))
		rover = (bp);


	return bp;
}


/* Fitment functions */

/* first fit */
static void *first_fit(size_t asize) 
{


	void *bp = list_start; 

	if (bp == NULL)
		return NULL;

	/* search routine traversing the linked list */
	while (/* (GET_SIZE(HDRP(bp)) != 0) && ((*SUCC(bp)) != NULL) */ bp != NULL ) {

		if ((asize <= GET_SIZE(HDRP(bp))))
		{
			return bp;
		}

		bp = *(SUCC(bp));
	}

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
static void place(void *bp, size_t asize) 
{

	size_t remainder = GET_SIZE(HDRP(bp)) - asize;
	/* size of remainder should match the general minimum size, also determined
	 * at mm_init(), because a split free block should contain the header,
	 * footer and both the pred and succ pointers. */
	size_t minimum_split = 3*DSIZE; // remainder

	if ( remainder >= (minimum_split) ) { // split
		
		/* shorten the allocated block; */
		/* ADDON_1: get status of previous and put that in too */
		PUT(HDRP(bp), PACK(asize, GET_ALLOC_PREV(HDRP(bp)) + 1));
		/* ADDON_1: don't include ftr */
		PUT(FTRP(bp), PACK(asize, GET_ALLOC_PREV(HDRP(bp)) + 1)); 

		/* take care that disabling this didnt create other place/split issues*/
		// removeBlockFromList(bp);

		if (bp == list_start) {
			list_start = *(SUCC(bp));
		} else {
			removeBlockFromList(bp);
		}
		
		/* cut remainder to leave a free block; previous is allocated (+2); can coalescing be done here? */
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(remainder, 0+2));
		PUT(FTRP(bp), PACK(remainder, 0+2));



		// prependBlockToList(bp, (unsigned int *)list_start);
		coalesce(bp);

		
	} else { // keep current block size
		/* store status of current block */
		/* ADDON_1: get status of previous as well. Necessarily allocated? Don't include ftr.*/
		PUT(HDRP(bp), PACK(GET_SIZE(HDRP(bp)), GET_ALLOC_PREV(HDRP(bp)) + 1));
		PUT(FTRP(bp), PACK(GET_SIZE(HDRP(bp)), 1)); 

		/* ADDON_1: inform next block that current is allocated */
		PUT(HDRP(NEXT_BLKP(bp)), PACK(GET_SIZE(HDRP(NEXT_BLKP(bp))), GET_ALLOC(HDRP(NEXT_BLKP(bp))) + 2));

		/* this again needs to properly place a block should a free one
		 * be at the start of the list */
		// removeBlockFromList(bp);

		if (bp == list_start) {
			list_start = *(SUCC(bp));
		} else {
			removeBlockFromList(bp);
		}

	}
	return;
}

/* a defrag routine, called at every free. Coalesces if a block is unallocated. */
static void defragment(void)
{

	void *bp = heap_listp;
	while (GET_SIZE(HDRP(bp))) {

		if (!(GET_ALLOC(HDRP(bp))))
			bp = coalesce(bp);
		
		bp = NEXT_BLKP(bp);
	}

}


/* linked list helpers */

/* insert a block at the start of the free list */
/* currently this fcn is called after the coalescing is done on adjacent blocks for each case */
static void prependBlockToList(void *bp, unsigned int *target)
{

	/* in case of coalescing with a free block already in the list */
	/* the issue is that, when prepending a block into which adjacent
	 * blocks were freed and coalesced, there is a chance that list_start
	 * pointed either at the previous or the next block, thus meaning that 
	 * it points somewhere inside the new block. 
	 * 
	 * The case that needs to be handled here is having the new block's 
	 * successor be a block that was coalesced into it, thus pointing into 
	 * itself. Failure to handle it eventually results in payload sticking 
	 * out of the heap, or payload overlap.
	 * 
	 * The tasks necessary for proper prepending depend on the case. This 
	 * case differentiation needs to be done somewhere; here, or in coalesce()
	 * which already differentiates between coalescing cases, so it might 
	 * be a more modular solution to do some of the work there. 

	 */

	/* the possible appending cases 

		~~~both neighbors allocated, no coalescing
		the middle bp is passed to this fcn
		the succ of the new head should just be the former list
		*(SUCC(bp)) = list_start;
		the prev of the former head list_start, if the former head exists, should be the current head
		*(PRED(list_start)) = bp;

		here prependBlockToList(bp, list_start as target)


		~~~prev allocated, next free
		here next is removed from list, coalesced, and it may be the former head (list_start)
		so the middle bp is passed to this fcn
		Problem: if next was the former head and the ONLY node, its prior removal also reset list_start to NULL,
		so it cannot be accessed via list_start
		then the new head succ should store the former head's succ ( even though bp!=list_start).
		*(SUCC(bp)) = *SUCC(list_start);
		and that one's pred has to be 
		*(PRED(*SUCC(list_start))) = bp;

		here prependBlockToList(bp, *SUCC(list_start) as target)

		but if the next is not the former head, (list_start doesnt point at it) then the new head succ is
		*(SUCC(bp)) = list_start; 
		and that one's prev is 
		*(PRED(list_start)) = bp;
		just like in the no coalescing case.

		here prependBlockToList(bp, list_start as target)








		~~~prev free, next allocated
		prev removed from list, coalesced, may be former head (list_start)
		the prev bp is passed to this fcn and bp==list_start
		once again the new head succ should be the succ of the former head
		*(SUCC(bp)) = *SUCC(list_start); // but it already is that anyways?
		and once again that one's pred has to be the current
		*(PRED(*SUCC(list_start))) = bp;

		here prependBlockToList(bp, *SUCC(list_start) as target)

		but if prev is not the former head... 
		*(SUCC(bp)) = list_start; 
		and that one's prev is 
		*(PRED(list_start)) = bp;
		just like in the no coalescing case.

		here prependBlockToList(bp, list_start as target)








		~~~both free
		here either prev, or next, or neither, could be list_start, but in either case the 
		prev block is the one where the new start of block is

		DANGER OF CIRCULAR LL?
		if either were list_start then the new head succ should be the succ of the former head
		*(SUCC(bp)) = *SUCC(list_start); // but it already is that anyways?
		and once again that one's pred has to be the current
		*(PRED(*SUCC(list_start))) = bp;

		here prependBlockToList(bp, *SUCC(list_start) as target)


		otherwise if neither are list_start
		here prependBlockToList(bp, list_start as target)

	*/


	/* ALWAYS: first node will have no predecessors since it is the start of the list */
	*(PRED(bp)) = NULL;


	*(SUCC(bp)) = target;

	/* should only run with list_start if neither neighbor were the list_start */
	if (target)
		*(PRED(target)) = bp;


	/* ALWAYS: the prepended block bp is the start of the list */
	list_start = bp;

	return;
}

/* remove free block from linked list */
/* Whichever adjacent free blocks are coalesced, they are disengaged from
 * the linked list and the list in that location should be re-knitted.
 * This may happen at either end of the LL; edge cases should avoid
 * de-referencing NULL ptrs. PRED/SUCC locations are always valid, but 
 * may contain NULL ptrs. Block removal from list should be accompanied 
 * by the patching of the list. */ 
static void removeBlockFromList(void *bp)
{


	/* maybe a guard, so that it does not try removing NULL, or prologue / epilogue
	 * because neither of them have pointer structures holding pred/succ, but since 
	 * both are marked as allocated, they should never be removed in coalesce() */

	/* if bp is the only block in LL, set list to empty */
	if ((*(PRED(bp)) == NULL) && (*(SUCC(bp)) == NULL)) {
		list_start = NULL;
		return;
	}

	/* predecessor to block should point at block's successor */
	/* so when the cond passes, the predcessor is not null but it is an inaccessible garbage value out of the heap.*/
	if (*(PRED(bp)) != NULL)
		*(SUCC(*(PRED(bp)))) = *(SUCC(bp));

	/* successor to block should point at block's predecessor*/
	if (*(SUCC(bp)) != NULL)
		*(PRED(*(SUCC(bp)))) = *(PRED(bp));

}








/* heap checker - where to call, what to report? 
 *
 * is every block in free list marked as free?
 * any contiguous free blocks that escaped coalescing?
 * is every free block actually in the free list?
 * do the pointers in the free list point to free blocks?
 * do the pointers in a heap block point to valid heap addresses?

 * do any allocated blocks overlap? mdriver tracks blocks via linked list,
 * where each node is constructed from a line (allocation request) in a 
 * trace file. given a known heap start, it can also track payloads and 
 * check if they overlap, as the driver calls mm_malloc and scans the LL 
 * struct nodes.
 */
static int mm_check(void)
{
	int heapstatus = 0;
	int liststatus = 0;

	printf("Heap start: %p; Heap end: %p\n", mem_heap_lo(), mem_heap_hi());

	/* printout of current heap morphology */
	if ((heapstatus = checkheap()) > 1) {

		printf("ERROR: Heap status is %d\n", heapstatus);
		exit(0);
	}

	printf("\n");


	/* printout of current free list */
	if ((liststatus = checklist()) > 1) {

		printf("ERROR: List status is %d\n", liststatus);
		exit(0);
	}

	printf("\n\n\n");

	/* all heap metrics OK */
	return 1;
}

static int checkheap(void)
{
	/* start from prologue block */
	void *prologue = (void *)((char *)heap_listp );
	void *bp;
	int errstatus = 1;

	printf("HEAP MORPHOLOGY\n");
	printf("Prologue block: "); printblock(prologue); printf(" - - -\n");
	for (bp = NEXT_BLKP(prologue); GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {

		printblock(bp);
		if ((errstatus = checkblock(bp)) > 1)
			return errstatus;

	}
	printf(" - - -\nEpilogue block: a: hdr[%d][%d] NO-FTR | addr: %p\n", GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp)), bp);

	/* heap morphology OK*/
	return 1; 	

}



static int checklist(void)
{

	void *bp;
	int errstatus = 1;

	printf("FREE LIST\n");
	printf("Free list head: %p\n - - - \n", list_start);

	/* empty list */
	if (list_start == NULL) {
		printf(" - - - \nEmpty list.\n\n\n\n");
		return 1;
	}

	for (bp = list_start; (/*(*SUCC(bp))*/bp != NULL); bp = *(SUCC(bp)) ) {

		printnode(bp);
		if ((errstatus = checknode(bp)) > 1)
			return errstatus;

	}

	printf(" - - - \nList finished.\n\n\n\n");

	/* list OK */
	return 1;

}



static void printblock(void *bp) 
{

	if (GET_ALLOC(HDRP(bp))) {

		/* format a/f: hdr[blk size, payload size] ftr[blk size, payload size]*/
		printf("a: hdr[%d][%d] ftr[%d][%d] | addr: %p\n", GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp)), GET_SIZE(FTRP(bp)), GET_ALLOC(FTRP(bp)), bp);


	} else {
		printf("f: hdr[%d][%d] ftr[%d][%d] | addr: %p\n", GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp)), GET_SIZE(FTRP(bp)), GET_ALLOC(FTRP(bp)), bp);

	}

	return;

}

static int checkblock(void *bp)
{
	int status = 1;

	if (GET_ALLOC(HDRP(bp))) /* allocated blocks don't need to have hdr == ftr */
		return status;

	if (!(GET_ALLOC(HDRP(bp)) == GET_ALLOC(FTRP(bp)))) {
		printf("HDR alloc not match that of FTR.\n");
		status+=2;
	}

	if (!(GET_SIZE(HDRP(bp)) == GET_SIZE(FTRP(bp)))) {
		printf("HDR size not match that of FTR.\n");
		status+=4;
	}

	return status;



	/* how is block alignment checked? */

}

static void printnode(void *bp)
{
	printf("f: hdr[%d][%d] pred[%p] succ[%p] ftr[%d][%d] | addr: %p\n", GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp)), *(PRED(bp)), *(SUCC(bp)), GET_SIZE(FTRP(bp)), GET_ALLOC(FTRP(bp)), bp);			

	return;
}

static int checknode(void *bp)
{
	int status = 1;

	if ((bp >= mem_heap_hi()) || (bp <= mem_heap_lo())) {
		printf("Successor ptr is out of bounds, %p \n", bp); 
		status+=2;
	}
	if (GET_ALLOC(HDRP(bp))) {
		printf("Allocated block is in free list, %p \n", bp); 
		status+=4;
	}

	return status;

}





