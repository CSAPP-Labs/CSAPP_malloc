/* heap checker with routines for checking heap-structure and list-structure */
#include "heapcheck.h"


/* heap checker driver */
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
		printf("ERROR: Allocator status is %d\n", status);
		// exit(0);
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
		if (!(GET_ALLOC(HDRP(bp))) && !(GET_ALLOC(HDRP(PREV_BLKP(bp))))) {
			printf("ERROR: Free block [%p] not coalesced to its previous neighbour [%p]. \n", bp, PREV_BLKP(bp));
			errstatus=4;
			exit(0);
		}

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

	for (bp = list_start; (bp != NULL); bp = *(SUCC(bp)) ) {

		if (verbose == 1)
			printnode(bp);

		/* check correctness of double linking */
		if (prev_bp && (*PRED(bp) != prev_bp)) {
			printf("ERROR: current block [%p] does not point at previous [%p] as its predecessor. \n", bp, prev_bp);	
			errstatus=4;	
		}

		if ((errstatus = checknode(bp)) > 1)
			printf("\n");

		/* check address ordering */
		if (prev_bp) {

			if (bp < prev_bp) {
				printf("ERROR: current block address [%p] smaller than previous [%p] at request nr [%d]. \n", bp, prev_bp, request_count);	
				errstatus=4;	
				// exit(0);			
			}

			if (bp == prev_bp) {
				printf("ERROR: current block address [%p] equal to previous [%p]. \n", bp, prev_bp);	
				errstatus=4;				
			}

		}

		prev_bp = bp;

		/* check for circular linked list here via hare/tortoise algorithm */
		if ((hare == NULL) || (*SUCC(hare) == NULL)) { /* hare reached the end, list not circular.*/
		} else {
			tortoise = *SUCC(tortoise);
			hare = *SUCC(*SUCC(hare)); /* also check if it runs out of heap bounds */
			if (tortoise == hare) {
				printf("ERROR: List is circular at [%p].\n", tortoise);
				errstatus=4;
				// exit(0); /* to avoid running forever */
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
		printf("a: hdr[%d][%d][prev:%d] ftr[%d][%d] | addr: %p\n", 
			GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp)), GET_ALLOC_PREV(HDRP((bp))), GET_SIZE(FTRP(bp)), GET_ALLOC(FTRP(bp)), bp);

	} else {
		printf("f: hdr[%d][%d][prev:%d] ftr[%d][%d] | addr: %p\n", 
			GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp)), GET_ALLOC_PREV(HDRP((bp))), GET_SIZE(FTRP(bp)), GET_ALLOC(FTRP(bp)), bp);

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



