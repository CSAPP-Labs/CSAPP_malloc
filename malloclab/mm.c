/*
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
 * ADDON_0: next_fit() thrown out for linked list designs.
 *
 * ADDON_1: modification to eliminate need for a footer in allocated blox.
 *
 *
 * Description of implementation #2: explicit free lists
 * 
 * The design is implemented as a layer on top of the implicit free list design.
 * Block headers and footers are preserved for the purpose of overview of heap
 * morphology. A doubly-linked list is maintained to track free blocks, by 
 * including predecessor and successor pointers within each free block, thus 
 * increasing the constraint of minimum block size to hdr+pred+succ+ftr. 
 * 
 * Design performance: overall util 46/60, thru 40/40. Design of double-linked 
 * free list with LIFO free block maintenance, best-fit algorithm tweaked to 
 * stop searching for a better fit after 100 scanned free blocks. Main short-
 * coming are the traces of reallocs, binaries, and coalescing, which have util 
 * of around 30/60. Increasing CHUNKSIZE over 1<<12 yields no significant changes, 
 * but it breaks an assumption by the realloc function specifically at 1<<10, 
 * likely hitting some edge condition. Below 1<<9, util improves up to 53/60 but
 * throughput drops below 35/40.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include "mm.h"
#include "memlib.h"
#include "mm_macros.c" /* macros for memory management. */

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
static int max_requests = 3000 * 10; 
static int verbose = 1;



/* declare helper fcns */
static void *extend_heap (size_t words);
static void *coalesce(void *bp);
static void place(void *bp, size_t asize);

static void defragment(void);

static void *first_fit(size_t asize);
static void *best_fit(size_t asize);

static size_t pad_size (size_t size);

/* linked list helpers */
static void prependBlockToList(void *bp, unsigned int *target);
static void removeBlockFromList(void *bp);

/* declare heap checker helpers */
static int mm_check(int verbose);
static int checkheap(int verbose);
static int checklist(int verbose);
static void printblock(void *bp);
static int checkblock(void *bp);
static void printnode(void *bp);
static int checknode(void *bp);
static int blockNotInList(void *ptr);




/* mm_init - initialize the malloc package */
int mm_init(void)
{
	/* initial empty heap */
	if ( (heap_listp = mem_sbrk(4*WSIZE))== (void *)-1)
		return -1;

	/* alignment padding */
	PUT(heap_listp, 0);

	/* prologue hdr + ftr + epilogue hdr */
	PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1+2));
	PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1+2)); 	
	PUT(heap_listp + (3*WSIZE), PACK(0, 1+2));

	/* Points at prologue, end of hdr start of ftr */
	heap_listp+=(2*WSIZE); 

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

    request_count++; /* keep count of malloc requests for debugging */
    asize = pad_size(size);

    /* debugging macros only execute if DEBUG is defined */
    INTERRUPT(request_count, max_requests);
	CHECK("Before allocation nr %d of size [%d]\n", verbose, request_count, asize);

    /* search for suitable block via some fit method, and allocate */
    if ((bp = best_fit(asize)) != NULL) {
    	place(bp, asize);

    	// CHECK("After allocation %d by fit fcn\n", verbose, request_count, NULL);

    	return bp;
    }

    /* defer coalescing to when allocation fails, then try allocating via fit again */
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

	// CHECK("After extension via extend_heap+coalesce, prior to placing.\n", verbose, NULL, NULL);
    
    place(bp, asize);

	// CHECK("After allocation via extend_heap+coalescing, after placing.\n", verbose, NULL, NULL);

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

    /* mark block as free */
	PUT(HDRP(ptr), PACK(size, 0+prev_alloc));
	PUT(FTRP(ptr), PACK(size, 0+prev_alloc));

	/* ADDON_1: inform next block that the current one is free */
	PUT(HDRP(NEXT_BLKP(ptr)), PACK(GET_SIZE(HDRP(NEXT_BLKP(ptr))), GET_ALLOC(HDRP(NEXT_BLKP(ptr)))));

	/* coalescing does list insertion and pred/succ ptr setup, otherwise it would need to be done here */
	coalesce(ptr);

	// CHECK("After freeing addr %p\n", verbose, ptr, NULL);
}

/*
 * mm_realloc - ptr must either be NULL, or a value returned 
 * by a previous call to malloc. size must be nonnegative.
 *
 * basic: 	Implemented simply in terms of mm_malloc and mm_free
 * Performance of basic realloc is bad; util of 26% and 34% for
 * the realloc tests 1 and 2 respectively.
 *
 * modified: dedicated implementation of realloc
 */
void *mm_realloc(void *ptr, size_t size)
{
	CHECK("At entry of realloc.\n", verbose, NULL, NULL);

	/* basic implementation of realloc */
/*
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = (size_t)(GET_SIZE(HDRP(ptr)));
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;

*/
    /* dedicated implementation of realloc */
	void *oldptr = ptr;
    void *newptr;
    void *prev_ptr;
    void *next_ptr;
	size_t copySize;
	size_t asize;

	size_t size_current;
	size_t size_previous;
	size_t size_next;

	/* generic case of NULL ptr */
    if (ptr == NULL)
    	return (mm_malloc(size));

    /* generic case of size == 0 */
    if (size == 0) {
    	mm_free(ptr);
    	return NULL; 
    }

    /* ptr not NULL; must have been returned by earlier call to mm_malloc(ptr). */
    copySize = (size_t)(GET_SIZE(HDRP(ptr)));
    asize = pad_size(size);
    if (copySize == asize)
    	return oldptr;

    /* Coalesce with neighbouring free block if the sum suffices for realloc */
    if (asize > copySize) {

    	prev_ptr = PREV_BLKP(ptr);
    	next_ptr = NEXT_BLKP(ptr);
		size_current = GET_SIZE(HDRP(ptr));
		size_previous = GET_SIZE(HDRP(prev_ptr));
		size_next = GET_SIZE(HDRP(next_ptr));

	    /* expansion towards higher addresses is preferred */
		if (!(GET_ALLOC(HDRP(next_ptr))) && ((size_current + size_next) >= asize)) {

	    	/* remove subsequent block from the list */
			if (next_ptr == list_start) {
				list_start = *(SUCC(next_ptr));
			} else {
				removeBlockFromList(next_ptr);
			}

			/* join it to the current block */
			PUT(HDRP(ptr), PACK((size_current+size_next), GET_ALLOC_PREV(HDRP(ptr)) + 1));
			PUT(FTRP(ptr), PACK((size_current+size_next), GET_ALLOC_PREV(HDRP(ptr)) + 1)); 

			place(oldptr, asize);
			return oldptr;    	
	    } 

	    /* expansion backwards towards lower addresses involves memmove() */
		if (!(GET_ALLOC(HDRP(prev_ptr))) && ((size_current+size_previous) >= asize)) {

	    	/* remove subsequent block from the list; predecessor of new list also to be updated */
			if (prev_ptr == list_start) {
				if (*(SUCC(prev_ptr)) != NULL)
					*(PRED(*(SUCC(prev_ptr)))) = NULL;
				list_start = *(SUCC(prev_ptr));
			} else {
				removeBlockFromList(prev_ptr);
			}
			
			/* move data to the start of the previous block, which would overwrite the current ptr hdr and prev ftr */
			memmove(prev_ptr, ptr, copySize);

			/* must be "coalesced" with previous before doing placement */
			PUT(HDRP(prev_ptr), PACK(size_current+size_previous, GET_ALLOC_PREV(HDRP(prev_ptr)) + 1));
			PUT(FTRP(prev_ptr), PACK(size_current+size_previous, GET_ALLOC_PREV(HDRP(prev_ptr)) + 1)); 
			
			place(prev_ptr, asize);
			return prev_ptr;    	
	    } 

	}   

    /* if all else fails, default to conventional allocation */
    newptr = mm_malloc(size);

     /* determine how much of the old block can be copied into the new */
    if (size < copySize)
      copySize = size;

  	/* copy the memory content once the block is allocated via newptr, free oldptr */
  	memcpy(newptr, oldptr, copySize);
  	mm_free(oldptr);

  	CHECK("Default realloc of %p to new block %p \n", verbose, ptr, newptr);

  	return newptr;

}



/* --------------- helper functions --------------- */

/* extend the heap with a new free block.
 * called when initializing the allocator, and when no block found.
 */
static void *extend_heap (size_t words)
{
	size_t size;
	char *bp;
	int prev_alloc; /* ADDON_1: status of block before epilogue */
	void *next_ptr;

	/* allocate even nr of words; mem_sbrk returns ptr at former epilogue */
	size = (words % 2)? ((words+1) * WSIZE) : (words * WSIZE);
	if ((long)(bp = mem_sbrk(size)) == -1)
		return NULL;

	/* ADDON_1: extract epilogue information before extension inserted */
	next_ptr = NEXT_BLKP(bp);
	prev_alloc = GET_ALLOC_PREV(HDRP(next_ptr));

	/* prep the header, footer and epilogue header */
	/* as the heap grows, the former epilogue block gets overwritten
	 * by the header of the new block. the next epilogue is appended
	 * to the end of the current block "array". */
	PUT(HDRP(bp), PACK(size, 0+prev_alloc));
	PUT(FTRP(bp), PACK(size, 0+prev_alloc));
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* epilogue's prev. not allocated */
	
	return coalesce(bp); /* coalesce if prev block free, must be done, because it sets up pred/succ */
}

/* Coalesce the free bp with free neighboring blocks if they are free. Maintain free list. */
static void *coalesce(void *bp) 
{
	/* status of neighbor blocks, size of current */
	/* ADDON_1: previous alloc status should always be checked at current block hdr */
	size_t prev_alloc = GET_ALLOC_PREV(HDRP(bp)) / 2; // ==(2 / 2) if allocated
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));

	/* newbp defines what is prepended to the list; target defines how to do it */
	unsigned int *target;
	void *newbp;
	void *nextbp;

	/* possible cases when coalescing with neighbors */
	if (prev_alloc & next_alloc) { /* both allocated */
		newbp = bp;
		target = list_start;
	} else if (prev_alloc & (!next_alloc)) { /* coalesce with next*/
		newbp = bp;
		nextbp = NEXT_BLKP(bp);

		size+= GET_SIZE(HDRP(nextbp));

		if ( nextbp == list_start ) {
			target = *SUCC(list_start);
		} else {
			target = (unsigned int *)list_start;
		}
		removeBlockFromList(nextbp);

		PUT(HDRP(bp), PACK(size, 0+2));
		PUT(FTRP(bp), PACK(size, 0+2)); 

	} else if ((!prev_alloc) & next_alloc) { /* coalesce with prev */
		newbp = PREV_BLKP(bp);		
		size+= GET_SIZE(FTRP(newbp));


		if ( newbp == list_start ) {
			target = *SUCC(list_start);
		} else {
			target = (unsigned int *)list_start;
		}
		removeBlockFromList(newbp);

		PUT(HDRP(newbp), PACK(size, 2+0));
		PUT(FTRP(bp), PACK(size, 2+0));

	} else { /* coalesce with both */
		newbp = PREV_BLKP(bp);		
		nextbp = NEXT_BLKP(bp);
		size+= GET_SIZE(FTRP(newbp)) + GET_SIZE(HDRP(nextbp));


		if ((newbp == list_start)) {
			removeBlockFromList(nextbp);
			target = *SUCC(list_start);

		} else if ( (nextbp == list_start) ) {
			removeBlockFromList(newbp);
			target = *SUCC(list_start);

		} else {
			removeBlockFromList(newbp);
			removeBlockFromList(nextbp);
			target = (unsigned int *)list_start;
		}

		PUT(HDRP(newbp), PACK(size, 2+0));
		PUT(FTRP(nextbp), PACK(size, 2+0));
	}

	/* set up pointers in new free block; place it into linked list */
	prependBlockToList(newbp, target);

	return newbp;
}

/* placement helper 
 * assumes that bp is free, and the block big enough for asize
 * returns bp which points to an allocated block, and may split the 
 * remainder off and attach it to the free list.
 */
static void place(void *bp, size_t asize) 
{
	size_t size_current = GET_SIZE(HDRP(bp));
	size_t remainder = size_current - asize;
	size_t minimum_split = minblock; 	/* the split remainder minimum is the minimum block size */

	if ( remainder >= (minimum_split) ) { /* split */
		/* block removal depends on where the block is */
		if (bp == list_start) {
			list_start = *(SUCC(bp));
		} else {
			removeBlockFromList(bp);
		}

		// CHECK("Realloc of size %d | inside place, before block macros \n", verbose, asize, NULL);

		/* shorten the allocated block; */
		/* ADDON_1: get status of previous and put that in too; ftr may be excluded */
		PUT(HDRP(bp), PACK(asize, GET_ALLOC_PREV(HDRP(bp)) + 1));
		PUT(FTRP(bp), PACK(asize, GET_ALLOC_PREV(HDRP(bp)) + 1)); 

		/* cut remainder to leave a free block; previous is allocated (+2) */
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(remainder, 0+2));
		PUT(FTRP(bp), PACK(remainder, 0+2));

		// CHECK("Realloc of size %d | inside place, after block macros \n", verbose, asize, NULL);

		/* coalesce if possible, attach to list */
		coalesce(bp);
		
	} else { /* keep current block size */
		/* block removal depends on where the block is */
		if (bp == list_start) {
			list_start = *(SUCC(bp));
		} else {
			removeBlockFromList(bp);
		}

		/* store status of current block */
		/* ADDON_1: get status of previous as well. Don't include ftr.*/
		PUT(HDRP(bp), PACK(size_current, GET_ALLOC_PREV(HDRP(bp)) + 1));
		PUT(FTRP(bp), PACK(size_current, 1)); 

		/* ADDON_1: inform next block that current is allocated */
		PUT(HDRP(NEXT_BLKP(bp)), PACK(GET_SIZE(HDRP(NEXT_BLKP(bp))), GET_ALLOC(HDRP(NEXT_BLKP(bp))) + 2));
	}

	return;
}

/* pad the requested block size */
static size_t pad_size (size_t size)
{
    if (size <= (2*DSIZE)) {
    	return minblock;
    } else {
    	/* round up to nearest 8, and add 4 for hdr; 8 if allocated block needs hdr+ftr */
    	return (((size + DSIZE + (DSIZE - 1))/DSIZE) * DSIZE);
    }
}

/* Fitment functions */

/* first fit: fast, but suboptimal heap utilization */
static void *first_fit(size_t asize) 
{
	void *bp = list_start; 
	if (bp == NULL)
		return NULL;

	/* search routine traversing the linked list till end */
	while (bp != NULL ) {

		if ((asize <= GET_SIZE(HDRP(bp)))) 
			return bp;
		
		bp = *(SUCC(bp));
	}
	return NULL;
}

/* best fit; bad performance with implicit list; entire list must be scanned */ 
/* unambiguously better utilization for all tests, but much worse throughput */
static void *best_fit(size_t asize) 
{
	void *bp = list_start; 
	void *candidate = NULL;
	int candidates_found = 0;
	int total_traversed = 0;
	size_t size_current;

	if (list_start == NULL)
		return NULL;

	/* look for the first candidate */
	for (; bp != NULL; bp = *(SUCC(bp))) {
		if ((asize <= GET_SIZE(HDRP(bp)))) {
			candidate = bp;
			candidates_found++;
			bp = *(SUCC(bp));
			break;
		}
	}

	if (candidate == NULL)
		return NULL;

	if (bp == NULL)
		return candidate;

	/* traverse free list to find a better candidate than the first */
	for (; bp != NULL; bp = *(SUCC(bp))) {
		total_traversed++;
		size_current = GET_SIZE(HDRP(bp));

		if ((asize == size_current)) 
			return bp;

		if ((asize < size_current) && (size_current < GET_SIZE(HDRP(candidate)))) {
			candidates_found++;
			candidate = bp;
		}

		if (total_traversed  > 100)
			return candidate;	
	}

	/* */
	if (candidates_found > 0) {
		return candidate;
	}
	else {
		return NULL;
	}

}


/* Defrag routine: scan entire heap and coalesce adjacent free blocks.
 * No need for deferred coalescing with a LIFO linked list + best fit; 
 * throughput grading is 40/40 without it */
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

/* prepend free block to free list; prepending target is resolved by caller to
 * avoid edge cases and and pointers pointing inside of a coalesced free block */
static void prependBlockToList(void *bp, unsigned int *target)
{
	/* ALWAYS: first node will have no predecessors since it is the start of the list */
	*(PRED(bp)) = NULL;

	/* successor to the prepended block depends on where the call is made */
	*(SUCC(bp)) = target;

	/* should only run with list_start if neither neighbor were the list_start */
	if (target)
		*(PRED(target)) = bp;

	/* ALWAYS: the prepended block bp is the start of the list */
	list_start = bp;

	return;
}

/* remove free block from linked list; repair the free list there */
static void removeBlockFromList(void *bp)
{
	void *predecessor = *(PRED(bp));
	void *successor = *(SUCC(bp));

	if (GET_ALLOC(HDRP(bp)))
		return;

	/* if bp is the only block in LL, set list to empty */
	if (((predecessor) == NULL) && ((successor) == NULL)) {
		list_start = NULL;
		
	} else {

		/* predecessor to block should point at block's successor */
		if ((predecessor) != NULL)
			*(SUCC(predecessor)) = successor;

		/* successor to block should point at block's predecessor*/
		if ((successor) != NULL)
			*(PRED(successor)) = predecessor;
	}

	return;
}


/* heap checker with routines for checking heap-structure and list-structure */
static int mm_check(int verbose)
{
	int heapstatus = 0;
	int liststatus = 0;
	int status = 1;

	if (verbose == 1) {printf("Heap start: %p; Heap end: %p\n", mem_heap_lo(), mem_heap_hi()); }

	/* printout of current heap morphology */
	/* checks if a free block is not in free list */
	if ((heapstatus = checkheap(verbose)) > 1) {

		printf("ERROR: Heap status is %d\n", heapstatus);
		status+=heapstatus;
		// exit(0);
	}

	if (verbose == 1) {printf("\n");}

	/* printout of current free list */
	if ((liststatus = checklist(verbose)) > 1) {

		printf("ERROR: List status is %d\n", liststatus);
		status+=liststatus;
		// exit(0);
	}

	if (verbose == 1) {printf("\n\n\n");}

	if (status > 1) {
		printf("ERROR: Allocator status is %d\n", liststatus);
		exit(0);
	}

	/* all heap metrics OK if status is 1 */
	return status;
}

/* errstatus type 4 if heap/block structure related*/
static int checkheap(int verbose)
{
	/* start from prologue block */
	void *prologue = (void *)((char *)heap_listp );
	void *bp;
	int errstatus = 0;

	if (verbose == 1) {
		printf("HEAP MORPHOLOGY\n");
		printf("Prologue block: "); printblock(prologue); printf(" - - -\n");
	}

	for (bp = NEXT_BLKP(prologue); GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {

		if (verbose == 1) 
			printblock(bp);

		if ((errstatus = checkblock(bp)) > 1) {
			printf("\n");
		}

		/* list related error */
		if (!(GET_ALLOC(HDRP(bp))) && blockNotInList(bp)) {
			printf("ERROR: Free block [%p] not in list. \n", bp);
			errstatus=4;
		}

		/* check for uncoalesced adjacent free blocks here */
		// degreeOfExternalFrag(startofheap);
	}

	if (verbose == 1) {
		printf(" - - -\nEpilogue block: a: hdr[%d][%d] NO-FTR | addr: %p\n", GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp)), bp);
	}

	/* heap morphology OK if returns 1 */
	return errstatus; 	
}

/* errstatus contributions 128 and greater are list-related */
static int checklist(int verbose)
{
	void *bp;
	void *prev_bp = NULL;
	int errstatus = 0;

	/* circular linked list hare/tortoise detection */
	void *hare = list_start;
	void *tortoise = list_start;

	if (verbose == 1) {
		printf("FREE LIST\nFree list head: %p\n - - - \n", list_start);		
	}

	/* empty list */
	if (list_start == NULL) {
		if (verbose == 1) {printf(" - - - \nEmpty list.\n\n\n\n");}
		return 1;
	}

	if (*PRED(list_start) != NULL) {
		printf("ERROR: list head has a predecessor. \n");
		errstatus=4;
	}

	for (bp = list_start; (/*(*SUCC(bp))*/bp != NULL); bp = *(SUCC(bp)) ) {
		
		if (prev_bp && (*PRED(bp) != prev_bp)) {
			printf("ERROR: current block [%p] does not point at previous [%p] as its predecessor. \n", bp, prev_bp);	
			errstatus=4;	
		}

		if (verbose == 1)
			printnode(bp);

		if ((errstatus = checknode(bp)) > 1)
			printf("\n");

		prev_bp = bp;

		/* check for circular linked list here via hare/tortoise algorithm */
		if ((hare == NULL) || (*SUCC(hare) == NULL)) { /* hare reached the end, list not circular.*/
		} else {
			tortoise = *SUCC(tortoise);
			hare = *SUCC(*SUCC(hare)); /* also check if it runs out of heap bounds */
			if (tortoise == hare) {
				printf("ERROR: List is circular at[%p].\n", tortoise);
				errstatus=4;
				exit(0); /* to avoid running forever */
			}
		}

	}

	if (verbose == 1)
		printf(" - - - \nList finished.\n\n\n\n");

	/* at least 1 error if status greater than 1 */
	return errstatus;
}

static void printblock(void *bp) 
{

	if (GET_ALLOC(HDRP(bp))) {
		/* format a/f: hdr[blk size, payload size] ftr[blk size, payload size]*/
		printf("a: hdr[%d][%d] ftr[%d][%d] | addr: %p\n", 
			GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp)), GET_SIZE(FTRP(bp)), GET_ALLOC(FTRP(bp)), bp);

	} else {
		printf("f: hdr[%d][%d] ftr[%d][%d] | addr: %p\n", 
			GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp)), GET_SIZE(FTRP(bp)), GET_ALLOC(FTRP(bp)), bp);

	}

	return;
}

static int checkblock(void *bp)
{
	int status = 0;

	if (GET_ALLOC(HDRP(bp))) // allocated blocks don't need to have hdr == ftr if the ftr is eliminated for allocation 
		return status;

	if (!(GET_ALLOC(HDRP(bp)) == GET_ALLOC(FTRP(bp)))) {
		printf("HDR alloc not match that of FTR.\n");
		status=2;
	}

	if (!(GET_SIZE(HDRP(bp)) == GET_SIZE(FTRP(bp)))) {
		printf("HDR size not match that of FTR.\n");
		status=2;
	}

	return status;

	/* block alignment needs payload info for each block */

}

static void printnode(void *bp)
{
	printf("f: hdr[%d][%d] pred[%p] succ[%p] ftr[%d][%d] | addr: %p\n", 
		GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp)), *(PRED(bp)), *(SUCC(bp)), GET_SIZE(FTRP(bp)), GET_ALLOC(FTRP(bp)), bp);
	
	return;
}

static int checknode(void *bp)
{
	int status = 0;

	/* does a block pointer point somewhere outside of the heap? */
	if ((bp >= mem_heap_hi()) || (bp <= mem_heap_lo())) {
		printf("Successor ptr is out of bounds: %p \n", bp); 
		status=3;
	}

	/* does the free list contain allocated blocks? */
	if (GET_ALLOC(HDRP(bp))) {
		printf("Allocated block is in free list, %p \n", bp); 
		status=3;
	}

	return status;
}

/*static int degreeOfExternalFrag(void* startofheap) 
{

}
*/

static int blockNotInList(void *ptr)
{
	void *bp;
	for (bp = list_start; (/*(*SUCC(bp))*/bp != NULL); bp = *(SUCC(bp)) ) {
		if (ptr == bp)
			return 0;
	}

	/* block not found */
	return 1;
}





