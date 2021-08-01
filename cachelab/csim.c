#define _GNU_SOURCE

/* Supporting libraries */
#include "cachelab.h"
#include <stdio.h> 	// contains file i/o
#include <getopt.h>
#include <unistd.h>	// for getopt cmdline arg processing
#include <stdlib.h>
#include <math.h>
#include <string.h>

/* Program description:
 * Simulate hit/miss behaviour of a cache memory on the input valgrind trace.
 * Command line input: ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace -a
 */
 
 // runtime outputs
static int HITCOUNT = 0; static int MISSCOUNT = 0; static int EVICTCOUNT = 0;
static char hit = 0; static char miss = 0; static char evict = 0; 
 
/* -h help command information string */
char *help_info = 
"Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file> \
\nOptions: \
\n  -h         Print this help message. \
\n  -v         Optional verbose flag. \
\n  -s <num>   Number of set index bits. \
\n  -E <num>   Number of lines per set. \
\n  -b <num>   Number of block offset bits. \
\n  -t <file>  Trace file. \
\n \
\n Examples: \
\n  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace \
\n  linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n";
 
 
/* Line structure */
typedef struct line {
    short valid;
    unsigned long tag;
    short num; // redundant, here to mark order in which node created
    // unsigned long block; // actual block memory access not simulated 
    struct line *previous;
    struct line *next;
} line_t;


/* Cache management functions: candidates for object class constructors, members */
void usage_recency(int query_set, line_t **cache_sets, line_t *current_line)
{

   /* printf("Cache set nr: %d\nPTR CURRENT ELEM: %p | PTR cache set: %p | PTR PREV: %p\n", query_set, current_line, cache_sets[query_set], current_line->previous); */
   
   // let previous line point at the one after the current (or NULL)
   (current_line->previous)->next = current_line->next;
   
   // the next line's previous should point at the current line's previous
   if (current_line->next) {(current_line->next)->previous = current_line->previous;}
   
   // let current line point at first line in set
   current_line->next = (cache_sets[query_set]);
   
   // current cache set's previous should also be repaired: point to ... current line itself?
   (cache_sets[query_set])->previous = current_line;
   
   // let the first line in the set be the current line
   (cache_sets[query_set]) = current_line;
   
   // (cache_sets[query_set])->previous->next = NULL;
   (cache_sets[query_set])->previous = NULL;
}

// maybe the queried set needs to be modulated modS before indexing the cache_sets array
void query_cache(int query_set, int query_tag, line_t **cache_sets) 
{
   line_t *current_line = cache_sets[query_set];
   if (!current_line) {printf("ERROR at start of query_cache.\n"); exit(0);}
   
   // traverse lines till last line in linked list
   while (current_line) {   
      if (current_line->valid) {      
         // found the line with matching tag, hit
         if (current_line->tag == query_tag) {
         
            // printf("[hit:INLIST]Set accessed: %d | Line accessed: %d\n", query_set, current_line->num);
            HITCOUNT++; hit = 1;
            
            // append line to front
            if (!(cache_sets[query_set] == current_line)) {usage_recency(query_set, cache_sets, current_line);}            
            return;
         }

         // end of linked list, lines occupied, no matches. need to evict LRU.
         if (current_line->next == NULL) {
         
            // overwite valid line
            current_line->tag = query_tag;
            
            // printf("[miss+evict:EOL]Set accessed: %d | Line accessed: %d\n", query_set, current_line->num);
            MISSCOUNT++; EVICTCOUNT++; miss = 1; evict = 1;
            // append line to front
            if (!(cache_sets[query_set] == current_line)) {usage_recency(query_set, cache_sets, current_line);}
            return;
         }
      } else {       
         // current line not valid. always fill first empty line encountered. if not already first, bring to front.
         current_line->valid = 1;
         current_line->tag = query_tag;
         
         MISSCOUNT++; miss = 1; 
         // bring to front if not already there
         if (!(cache_sets[query_set] == current_line)) {usage_recency(query_set, cache_sets, current_line);}
         // printf("[miss:FIRST_EMPTY]Set accessed: %d | Line accessed: %d\n", query_set, current_line->num);   
         return; 
      } 
      
      current_line = current_line->next;        
   }
}

void data_load(int set, int tag, line_t **cache_sets)
{
   query_cache(set, tag, cache_sets);  
}

void data_store(int set, int tag, line_t **cache_sets)
{
   // store has same effect as load in the simulation
   data_load(set, tag, cache_sets);
}

void cache_controller(char operation, unsigned long set, unsigned long tag, unsigned long byte_offset, line_t **cache_sets)
{
    if (operation == 'M') {
    // load, then store
       data_load(set, tag, cache_sets);
       data_store(set, tag, cache_sets);
    }

    if (operation == 'L') {
    // load
       data_load(set, tag, cache_sets);
    }
    
    if (operation == 'S') {
    // store
       data_store(set, tag, cache_sets);
    } 
}

// recursive line creation (functioning)
line_t* create_line_list (int E, line_t* previous_line) 
{
   if ( E > 0 ) {
      line_t * current_line = NULL;
      current_line = (line_t *) malloc(sizeof(line_t));
      if (current_line == NULL) {printf("ERROR while creating linked list node.\n"); exit(0);}
      current_line->valid = 0;
      current_line->tag = 0;
      
      if (previous_line) { current_line->previous = previous_line;} else {current_line->previous = NULL;}
      current_line->next = create_line_list(E-1, current_line);
      return current_line;      
   }  
   return NULL;
}

/* cleanup */
void free_node(line_t *current_node)
{
   if (current_node) {
      free_node(current_node->next);
      free(current_node);
   }
}

void free_cache_memory(line_t **cache_sets, int sets)
{
   // free each cache set's linked list of lines
   for (int i = 0 ; i < sets; i++)
      free_node(cache_sets[sets]);

   // free the cache sets ptr array
   free(cache_sets);
}

// scan the list to demonstrate the next and previous ptrs are accessible
void cache_scan(line_t **cache_sets, int sets)
{
   line_t *current_line = NULL;
   for (int i = 0; i < sets; i++) {   
	current_line = cache_sets[i];
	printf("Set: %d\nPTR IN CACHE ELEMENT: %p\n", i, cache_sets[i]); 
	printf("First line address: %p | nextptr: %p | prevptr: %p\n", current_line, current_line->next, current_line->previous);  
	current_line = current_line->next;
	while (current_line->next) {
	   printf("Currentptr: %p | nextptr: %p | prevptr: %p\n", current_line, current_line->next, current_line->previous);
	   current_line = current_line->next;
	}
	printf("Last line address: %p | nextptr: %p | prevptr: %p\n", current_line, current_line->next, current_line->previous);  
   }
   printf("End of scan -----------------------\n\n");

}

// iterative lines in set (functioning)
/*
line_t* create_line_list (int E) 
{
   line_t * current_line = NULL;
   line_t * next_line = NULL;
   
   // line numbering
   int line_count = E;
     
   for (int i = 0; i < E; i++) {
      line_count--;
      next_line = current_line;
      current_line = (line_t *) malloc(sizeof(line_t));
      if (current_line == NULL) {printf("ERROR while creating linked list node.\n"); exit(0);}   
      current_line->valid = 0;
      current_line->tag = -1;
      current_line->next = next_line; // make current line point to previously created one
      current_line->num = line_count;      
      // if previously created line exists, make it point to this line as its predecessor.
      if (next_line) {next_line->previous = current_line;}
   }
   
   // the final current line is the LL head
   current_line->previous = NULL;   
   return current_line;
}
*/

line_t **create_cache (int S, int E) 
{
   line_t **cache_sets = NULL;
   // cache array of LL pointers
   cache_sets = (line_t**) malloc (S * sizeof(line_t*));
   if (cache_sets == NULL) {
      printf("Memory not allocated.\n");
      exit(0);   
   }
   // each array element... contains the ptr of the starting line of linked list? 
   for (int i = 0; i < S; i++) 
      // cache_sets[i] = create_line_list(E); 
      cache_sets[i] = create_line_list(E, NULL);
      
   return cache_sets;
}

 
int main(int argc, char *argv[])
{
   // system parameters
   int m = 64;
   
   // command line input parameters
   int verbosity = 0;
   int s = -1;
   int E = -1;
   int b = -1;
   char *filename = 0;

   // derived parameters
   int t, S/*, B*/;
   line_t **cache_sets = NULL; 
   
   // VALGRIND trace line variables
   char operation;
   char *hex_address = NULL;
   unsigned long address, set, tag, byte_offset;
   unsigned long set_mask, tag_mask, block_mask;
   
   // file variables
   char *line_buf = NULL;
   char line_copy[20];
   size_t line_buf_size = 0;
   int line_count = 0;
   ssize_t line_size;
   FILE *tracefile;

   /* Cmdline input flags, check flag and arg validity */
   int c;
   opterr = 0;	// turns off getopt error reporting
   while ((c = getopt(argc, argv, "hvs:E:b:t:")) != -1) 
   {
      switch(c){
         case 'h':
         // If -h flagged, show help options only, exit.
            printf("%s", help_info);
            return 0;
            // break;
         case 'v':  // OPTIONAL: set verbosity.
	     verbosity = 1;
            break;
         case 's':  // set # of set index bits and proceed.
            s = atoi(optarg);           
            break;
         case 'E':  // set # of lines per set and proceed.
            E = atoi(optarg);            
            break;
         case 'b':  // set # of block offset bits and proceed.
            b = atoi(optarg);            
            break;
         case 't':  // extract filename from cmdline argument, proceed.
            filename = optarg;
            break;
         case '?':  // invalid option
	    if ( (c != 's') && (c != 'E') && (c != 'b') && (c != 't') ) {
		    printf("./csim: invalid option -- '%c'\n", optopt);
		    printf("%s", help_info);
		    return -1;
            }
            // break;
         default:
	    // printf("./csim: invalid option -- '%c'\n", optopt);
	    // return -1;
	    break;		
      }
   }
   if ( s < 1 || E < 1 || b < 1 || filename == 0 ) {
       printf("Missing required command line argument\n");
       printf("%s", help_info);
       return -1;   
   }
     
   // printf("Verbosity:%d | s:%d | E:%d | b:%d | filename:%s\n\n\n\n", verbosity, s, E, b, filename);
   /* derive cache set parameters based on input: address structure m=t+s+b */
   t = m - b - s;
   S = 2<<(s-1);
   // B = 2<<b;
   set_mask = tag_mask = block_mask = 0xffffffffffffffff;
   set_mask = ~(set_mask<<s);
   block_mask = ~(block_mask<<b);
   tag_mask = ~(tag_mask<<t);
   
   /* Allocate storage for cache data structures using malloc. */
   cache_sets = create_cache(S, E);
   
   /* check cache structure if needed */
   // cache_scan(cache_sets, S);
   
   /* Open and process the valgrind trace file. Each line is: [space]operation address,size */
   tracefile = fopen(filename, "r");
   if (tracefile) {
       do
       {
           line_size = getline(&line_buf, &line_buf_size, tracefile);    
           // skip all Instruction lines; guard for do/while off by 1
           if (line_buf[0] == 'I' || line_size < 1) 
               continue;
               
           line_count++;                      
           // operation character: it is always element 1
           operation = line_buf[1];
           
           // address: from element 3 inclusive until ,
           strcpy(line_copy, line_buf); // avoid losing line due to strtok
           hex_address = strtok(&(line_buf[3]), ",");
           
           // convert hex address to int; base specified as 16
           address = (unsigned long)strtol(hex_address, NULL, 16);
           // printf("operation %c | hex %s | decimal %ld\n", operation, hex_address, address);
           
           // extract set, tag, byte offset
           set = set_mask & (address>>b);
           tag = tag_mask & (address>>(b+s));
           byte_offset = block_mask & address;
           // printf("set %ld | tag %ld | byte_offset: %ld\n", set, tag, byte_offset);
           
           hit = 0; miss = 0; evict = 0;
           cache_controller(operation, set, tag, byte_offset, cache_sets);
           
           if (verbosity) {
               // trace line without newline, without initial space
               line_copy[strlen(line_copy) - 1] = '\0';
               printf("%s", &(line_copy[1]));
                              
               if (miss) printf("%s", " miss");
               if (evict) printf("%s", " eviction");               
               if (hit) printf("%s", " hit");               
               printf("%c", '\n');           
           }            

       } while (line_size >= 0);
       
       free(line_buf);
       line_buf = NULL;
       fclose(tracefile);
   }

   /* Free cache simulator memory */
   free_cache_memory(cache_sets, S);  

   printSummary(HITCOUNT, MISSCOUNT, EVICTCOUNT);
   return 0;
}


