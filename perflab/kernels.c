/********************************************************
 * Kernels to be optimized for the CS:APP Performance Lab
 ********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "defs.h"

/* 
 * Please fill in the following team struct 
 */
team_t team = {
    "AN",              /* Team name */

    "AAAA NNNN",     /* First member full name */
    "bovik@nowhere.edu",  /* First member email address */

    "",                   /* Second member full name (leave blank if none) */
    ""                    /* Second member email addr (leave blank if none) */
};

/***************
 * ROTATE KERNEL
 ***************/

/******************************************************
 * Your different versions of the rotate kernel go here
 ******************************************************/

/* 
 * naive_rotate - The naive baseline version of rotate 
 */
char naive_rotate_descr[] = "naive_rotate: Naive baseline implementation";
void naive_rotate(int dim, pixel *src, pixel *dst) 
{
    int i, j;

    for (i = 0; i < dim; i++)
	for (j = 0; j < dim; j++)
	    dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
}

/* 
 * rotate - Your current working version of rotate
 * IMPORTANT: This is the version you will be graded on
 */


// Only edit this file. Allowed to define macros, more globals, other fcns.
// All images are NxN squares, where N is a multiple of 32.
// RIDX(i,j,n) ((i)*(n)+(j))


// maybe the reduced mult edit yields no improvement because 
// multiplying by powers of 2 is converted into shifts and adds anyways.
char rotate_descr_1[] = "rotate: reduced mult, code motion";
void rotate_1(int dim, pixel *src, pixel *dst) 
{
    int i, j;
    
    int dims = dim*dim - dim;
    int dimj = 0;
    int dimi = 0;

    for (i = 0; i < dim; i++) {
        dimj = 0;
	for (j = 0; j < dim; j++) {
	    dst[ dims - dimj  + i ] = src[ dimi + j];
	    dimj += dim;
	    }
        dimi += dim;
    }
}



// Doing 2x1 vector meaning 2 loads, 2 stores, utilizing more fcnl units.
char rotate_descr_2[] = "rotate_2: operate on 2x1 submatrix";
void rotate_2(int dim, pixel *src, pixel *dst) 
{
    int i, j;

    for (i = 0; i < dim; i++)
	for (j = 0; j < dim; j+=2) {
	    dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
	    dst[RIDX(dim-1-j-1, i, dim)] = src[RIDX(i, j+1, dim)];
	    }
}

// 2x2 submatrix means 4 loads, 4 stores, improvement on 2x1; Mean=5
// Not clear if the improvement is due to the increase in 
// temporal+spatial locality, or use of multiple load/store functional
// units in an unrolling with no accumulators. 
// There's no kxk unrolling since each element requires one load 
// and one store anyways; nothing to accumulate.
// and a 2x4 yields an even bigger improvement at about Mean=5.9
char rotate_descr_3[] = "rotate_3: operate on 2x4 submatrix unrolled";
void rotate_3(int dim, pixel *src, pixel *dst) 
{
    int i, j;

    for (i = 0; i < dim; i+=2)
	for (j = 0; j < dim; j+=4) {
	    dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
	    dst[RIDX(dim-1-j-1, i, dim)] = src[RIDX(i, j+1, dim)];
	    dst[RIDX(dim-1-j, i+1, dim)] = src[RIDX(i+1, j, dim)];
	    dst[RIDX(dim-1-j-1, i+1, dim)] = src[RIDX(i+1, j+1, dim)];

	    dst[RIDX(dim-1-j-2, i, dim)] = src[RIDX(i, j+2, dim)];
	    dst[RIDX(dim-1-j-2-1, i, dim)] = src[RIDX(i, j+2+1, dim)];
	    dst[RIDX(dim-1-j-2, i+1, dim)] = src[RIDX(i+1, j+2, dim)];
	    dst[RIDX(dim-1-j-2-1, i+1, dim)] = src[RIDX(i+1, j+2+1, dim)];  
	    
	    }
}



// What the Chinese student did was more akin to spatial locality increase; 
// accessing multiple rows in parallel in one loop iteration.
// Hence the "block" approach doesn't make sense at all; no need to look
// at neighbour pixels for increased temporal locality. The task here is 
// to process as many pixels in a row as possible, thus increasing spatial
// locality.
// with p=8 rows processed in parallel, Mean=12
// with p=16, Mean = 15.
// with p=32, Mean = 15. Diminishing returns here.


#define copyrow(a) (dst[RIDX(dim-1-j, i+a, dim)] = src[RIDX(i+a, j, dim)])

char rotate_descr_4[] = "rotate_4: operate on p rows simultaneously";
void rotate_4(int dim, pixel *src, pixel *dst) 
{
    int i, j;
    
    int p = 16;
    

    for (i = 0; i < dim; i+=p)
	for (j = 0; j < dim; j++) {
	    copyrow(0); copyrow(1); copyrow(2); copyrow(3);	
	    copyrow(4); copyrow(5); copyrow(6); copyrow(7);
	    copyrow(8); copyrow(9); copyrow(10); copyrow(11);
	    copyrow(12); copyrow(13); copyrow(14); copyrow(15);
	    /*
	    copyrow(16); copyrow(17); copyrow(18); copyrow(19);	
	    copyrow(20); copyrow(21); copyrow(22); copyrow(23);
	    copyrow(24); copyrow(25); copyrow(26); copyrow(27);
	    copyrow(28); copyrow(29); copyrow(30); copyrow(31);
	    */
	    
    }
}

// Attempt to make use of the fact that in this use case, N is 
// a multiple of 32. Try to work in blocks of pxp. Unlike with
// the blocking technique of matrix multiply, this may not need
// a 5th inner loop, because it does not deal with rowxcol mult.

// doing a 2x2 via inner loops yields much worse performance than
// with hard-coded 2x2 above.
// doing a 8x8 is better, but still worse than hardcoded 2x2, which
// implies that the 2x2 does indeed make use of multiple fcnl units.
// 16x16 and 32x32 are even worse than 8x8.

// Attempt to combine using multiple fcnl units as in v3, and blocking
// by operating on submatrices on inner loop.
// So far the best performance when using 32x32. Mean=8.7
// presumably more inner loop unrolling would be even better.

char rotate_descr_5[] = "rotate_5: 32x32 submatrix, AND 2x4 unroll within";
void rotate_5(int dim, pixel *src, pixel *dst) 
{
    int i, j, ii, jj;
    
    int p = 32;
    

    for (ii = 0; ii < dim; ii+=p)
	for (jj = 0; jj < dim; jj+=p)
	    for (i = ii; i < (ii+p); i+=2)
	    	for (j = jj; j < (jj+p); j+=4) {
	    dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
	    dst[RIDX(dim-1-j-1, i, dim)] = src[RIDX(i, j+1, dim)];
	    dst[RIDX(dim-1-j, i+1, dim)] = src[RIDX(i+1, j, dim)];
	    dst[RIDX(dim-1-j-1, i+1, dim)] = src[RIDX(i+1, j+1, dim)];

	    dst[RIDX(dim-1-j-2, i, dim)] = src[RIDX(i, j+2, dim)];
	    dst[RIDX(dim-1-j-2-1, i, dim)] = src[RIDX(i, j+2+1, dim)];
	    dst[RIDX(dim-1-j-2, i+1, dim)] = src[RIDX(i+1, j+2, dim)];
	    dst[RIDX(dim-1-j-2-1, i+1, dim)] = src[RIDX(i+1, j+2+1, dim)];  
	  
	    }
}





 
char rotate_descr[] = "rotate: Current working version";
void rotate(int dim, pixel *src, pixel *dst) 
{
    rotate_4(dim, src, dst);
}

/*********************************************************************
 * register_rotate_functions - Register all of your different versions
 *     of the rotate kernel with the driver by calling the
 *     add_rotate_function() for each test function. When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.  
 *********************************************************************/

void register_rotate_functions() 
{
    // add_rotate_function(&naive_rotate, naive_rotate_descr);   
    add_rotate_function(&rotate, rotate_descr);   
    /* ... Register additional test functions here */
    /*
    add_rotate_function(&rotate_1, rotate_descr_1);
    add_rotate_function(&rotate_2, rotate_descr_2);
    add_rotate_function(&rotate_3, rotate_descr_3);
    add_rotate_function(&rotate_4, rotate_descr_4);
    add_rotate_function(&rotate_5, rotate_descr_5);
    */
    add_rotate_function(&rotate_4, rotate_descr_4);
    add_rotate_function(&rotate_5, rotate_descr_5);

}


/***************
 * SMOOTH KERNEL
 **************/

/***************************************************************
 * Various typedefs and helper functions for the smooth function
 * You may modify these any way you like.
 **************************************************************/

/* A struct used to compute averaged pixel value */
typedef struct {
    int red;
    int green;
    int blue;
    int num;
} pixel_sum;

/* Compute min and max of two integers, respectively */
static int min(int a, int b) { return (a < b ? a : b); }
static int max(int a, int b) { return (a > b ? a : b); }

/* 
 * initialize_pixel_sum - Initializes all fields of sum to 0 
 */
static void initialize_pixel_sum(pixel_sum *sum) 
{
    sum->red = sum->green = sum->blue = 0;
    sum->num = 0;
    return;
}

/* 
 * accumulate_sum - Accumulates field values of p in corresponding 
 * fields of sum 
 */
static void accumulate_sum(pixel_sum *sum, pixel p) 
{
    sum->red += (int) p.red;
    sum->green += (int) p.green;
    sum->blue += (int) p.blue;
    sum->num++;
    return;
}


/*
// NEW: 3x1 accumulation. does not need the num counter since it is 
// known that a mid pixel is in a 3x3 neighbourhood.
static void assign_sums_to_pixel(pixel *current_pixel, pixel_sum *sum1, pixel_sum *sum2, pixel_sum *sum3) 
{
    current_pixel->red = (unsigned short)((sum1->red + sum2->red + sum3->red)/9);
    current_pixel->green = (unsigned short)((sum1->green + sum2->green + sum3->green)/9);
    current_pixel->blue = (unsigned short)((sum1->blue + sum2->blue + sum3->blue)/9);
    return;
}
*/


/* 
 * assign_sum_to_pixel - Computes averaged pixel value in current_pixel 
 */
static void assign_sum_to_pixel(pixel *current_pixel, pixel_sum sum) 
{
    current_pixel->red = (unsigned short) (sum.red/sum.num);
    current_pixel->green = (unsigned short) (sum.green/sum.num);
    current_pixel->blue = (unsigned short) (sum.blue/sum.num);
    return;
}

/* 
 * avg - Returns averaged pixel value at (i,j) 
 */
static pixel avg(int dim, int i, int j, pixel *src) 
{
    int ii, jj;
    pixel_sum sum;
    pixel current_pixel;

    initialize_pixel_sum(&sum);
    for(ii = max(i-1, 0); ii <= min(i+1, dim-1); ii++) 
	for(jj = max(j-1, 0); jj <= min(j+1, dim-1); jj++) 
	    accumulate_sum(&sum, src[RIDX(ii, jj, dim)]);

    assign_sum_to_pixel(&current_pixel, sum);
    return current_pixel;
}



// averaging for edge case pixels; no accumulating, though
// it would benefit from accumulators in a more detailed
// optimization attempt for each edge type. 
// top bottom left right: tried, barely any improvement.

// this best kept as avg corner.
static pixel avg_edge(int dim, int i, int j, pixel *src) 
{
    int ii, jj;
    pixel_sum sum;
    pixel current_pixel;

    initialize_pixel_sum(&sum);
    for(ii = max(i-1, 0); ii <= min(i+1, dim-1); ii++) 
	for(jj = max(j-1, 0); jj <= min(j+1, dim-1); jj++) 
	    accumulate_sum(&sum, src[RIDX(ii, jj, dim)]);

    assign_sum_to_pixel(&current_pixel, sum);
    return current_pixel;
}

static pixel avg_top(int dim, int i, int j, pixel *src) 
{
    
    // Accumulator sums for pixel averaging.
    // Try accumulating for each row in 2x3.
    
    pixel current_pixel;
    
    int red = 0;
    int green = 0;
    int blue = 0;
    
  
    pixel *left = &(src[RIDX(i, j-1, dim)]);
    pixel *mid = &(src[RIDX(i, j, dim)]);
    pixel *right = &(src[RIDX(i, j+1, dim)]);
    red += (left->red + mid->red + right->red);
    green += (left->green + mid->green + right->green);
    blue += (left->blue + mid->blue + right->blue);
    
    left = &(src[RIDX(i+1, j-1, dim)]);
    mid = &(src[RIDX(i+1, j, dim)]);
    right = &(src[RIDX(i+1, j+1, dim)]);
    red += (left->red + mid->red + right->red);
    green += (left->green + mid->green + right->green);
    blue += (left->blue + mid->blue + right->blue);   
    current_pixel.red = red/6;
    current_pixel.green = green/6;
    current_pixel.blue = blue/6;
    
    return current_pixel;
}

static pixel avg_bottom(int dim, int i, int j, pixel *src) 
{
    
    // Accumulator sums for pixel averaging.
    // Try accumulating for each row in 2x3.
    
    pixel current_pixel;
    
    int red = 0;
    int green = 0;
    int blue = 0;
    
  
    pixel *left = &(src[RIDX(i-1, j-1, dim)]);
    pixel *mid = &(src[RIDX(i-1, j, dim)]);
    pixel *right = &(src[RIDX(i-1, j+1, dim)]);
    red += (left->red + mid->red + right->red);
    green += (left->green + mid->green + right->green);
    blue += (left->blue + mid->blue + right->blue);
    
    left = &(src[RIDX(i, j-1, dim)]);
    mid = &(src[RIDX(i, j, dim)]);
    right = &(src[RIDX(i, j+1, dim)]);
    red += (left->red + mid->red + right->red);
    green += (left->green + mid->green + right->green);
    blue += (left->blue + mid->blue + right->blue);  
    
    current_pixel.red = red/6;
    current_pixel.green = green/6;
    current_pixel.blue = blue/6;
    
    return current_pixel;
}

static pixel avg_left(int dim, int i, int j, pixel *src) 
{
    
    // Accumulator sums for pixel averaging.
    // Try accumulating for each row in 3x2.
    
    pixel current_pixel;
    
    int red = 0;
    int green = 0;
    int blue = 0;
 
    
    pixel *mid = &(src[RIDX(i-1, j, dim)]);
    pixel *right = &(src[RIDX(i-1, j+1, dim)]);
    red += (mid->red + right->red);
    green += (mid->green + right->green);
    blue += (mid->blue + right->blue);
  
    mid = &(src[RIDX(i, j, dim)]);
    right = &(src[RIDX(i, j+1, dim)]);
    red += (mid->red + right->red);
    green += (mid->green + right->green);
    blue += (mid->blue + right->blue); 
    
    mid = &(src[RIDX(i+1, j, dim)]);
    right = &(src[RIDX(i+1, j+1, dim)]);
    red += (mid->red + right->red);
    green += (mid->green + right->green);
    blue += (mid->blue + right->blue);   
    
    current_pixel.red = red/6;
    current_pixel.green = green/6;
    current_pixel.blue = blue/6;
    
    return current_pixel;
}

static pixel avg_right(int dim, int i, int j, pixel *src) 
{
    int ii;
    
    // Accumulator sums for pixel averaging.
    // Try accumulating for each row in 3x3.
    
    pixel current_pixel;
    
    int red = 0;
    int green = 0;
    int blue = 0;
    
    for(ii = i-1; ii <= (i+1); ii++)  {
    
    	pixel *left = &(src[RIDX(ii, j-1, dim)]);
    	pixel *mid = &(src[RIDX(ii, j, dim)]);
    	red += (left->red + mid->red);
    	green += (left->green + mid->green);
    	blue += (left->blue + mid->blue);
      
    }
    
    current_pixel.red = red/6;
    current_pixel.green = green/6;
    current_pixel.blue = blue/6;
    
    return current_pixel;
}


// averaging for middle pixel cases, which constitute most
// of an image's pixels. Should not need to check for edges.
// unpacking the sum and avg functions helps greatly with
// the separate loops smooth_3. 
// Unrolling for accumulation of r, g, b of 3x3 submatrix

/*
static pixel avg_mid(int dim, int i, int j, pixel *src) 
{
    int ii;

    pixel current_pixel;
    
    int red = 0;
    int green = 0;
    int blue = 0;
    
    for(ii = i-1; ii <= (i+1); ii++)  {
    
    	pixel *left = &(src[RIDX(ii, j-1, dim)]);
    	pixel *mid = &(src[RIDX(ii, j, dim)]);
    	pixel *right = &(src[RIDX(ii, j+1, dim)]);
    	red += (left->red + mid->red + right->red);
    	green += (left->green + mid->green + right->green);
    	blue += (left->blue + mid->blue + right->blue);
      
    }
    
    current_pixel.red = red/9;
    current_pixel.green = green/9;
    current_pixel.blue = blue/9;
    
    return current_pixel;
}

*/

// unpacking all accesses out of the loop to avoid branch
// prediction penalty, since only 3 iterations at most. 
// the improvement on simple unrolling is huge; Mean=25+
static pixel avg_mid_unpacked(int dim, int i, int j, pixel *src) 
{
    
    // Accumulator sums for pixel averaging.
    // Try accumulating for each row in 3x3.
    
    pixel current_pixel;
    
    int red = 0;
    int green = 0;
    int blue = 0;
 
    pixel *left = &(src[RIDX(i-1, j-1, dim)]);
    pixel *mid = &(src[RIDX(i-1, j, dim)]);
    pixel *right = &(src[RIDX(i-1, j+1, dim)]);
    red += (left->red + mid->red + right->red);
    green += (left->green + mid->green + right->green);
    blue += (left->blue + mid->blue + right->blue);
    
    left = &(src[RIDX(i, j-1, dim)]);
    mid = &(src[RIDX(i, j, dim)]);
    right = &(src[RIDX(i, j+1, dim)]);
    red += (left->red + mid->red + right->red);
    green += (left->green + mid->green + right->green);
    blue += (left->blue + mid->blue + right->blue);   

    left = &(src[RIDX(i+1, j-1, dim)]);
    mid = &(src[RIDX(i+1, j, dim)]);
    right = &(src[RIDX(i+1, j+1, dim)]);
    red += (left->red + mid->red + right->red);
    green += (left->green + mid->green + right->green);
    blue += (left->blue + mid->blue + right->blue);   
    
    current_pixel.red = red/9;
    current_pixel.green = green/9;
    current_pixel.blue = blue/9;
    
    return current_pixel;
}

/******************************************************
 * Your different versions of the smooth kernel go here
 ******************************************************/

/*
 * naive_smooth - The naive baseline version of smooth 
 */
char naive_smooth_descr[] = "naive_smooth: Naive baseline implementation";
void naive_smooth(int dim, pixel *src, pixel *dst) 
{
    int i, j;

    for (i = 0; i < dim; i++)
	for (j = 0; j < dim; j++)
	    dst[RIDX(i, j, dim)] = avg(dim, i, j, src);
}

// smooth with only rotate optimizations. Performance identical to naive
// implementation.
char smooth_descr_1[] = "smooth_1: only with optimizations from rotate. ";
void smooth_1(int dim, pixel *src, pixel *dst) 
{
    int i, j, ii, jj;
    
    int p = 32;
    

    for (ii = 0; ii < dim; ii+=p)
	for (jj = 0; jj < dim; jj+=p)
	    for (i = ii; i < (ii+p); i+=2)
	    	for (j = jj; j < (jj+p); j+=4) {
	    	    dst[RIDX(i, j, dim)] = avg(dim, i, j, src);
	    	    dst[RIDX(i, j+1, dim)] = avg(dim, i, j+1, src);
	    	    dst[RIDX(i+1, j, dim)] = avg(dim, i+1, j, src);
	    	    dst[RIDX(i+1, j+1, dim)] = avg(dim, i+1, j+1, src);
	    	    
	    	    dst[RIDX(i, j+2, dim)] = avg(dim, i, j+2, src);
	    	    dst[RIDX(i, j+2+1, dim)] = avg(dim, i, j+2+1, src);
	    	    dst[RIDX(i+1, j+2, dim)] = avg(dim, i+1, j+2, src);
	    	    dst[RIDX(i+1, j+2+1, dim)] = avg(dim, i+1, j+2+1, src); 
	  
	    }
}

// smooth using distinct averaging functions, conditionally selected
/*
char smooth_descr_2[] = "smooth_2: avg for edge and mid cases, with cond.";
void smooth_2(int dim, pixel *src, pixel *dst) 
{
    int i, j;
    int isEdge = 0;
    
    // a pointer requires initializing a variable on the stack, in
    // order to have something to point at. each time it is accessed, 
    // that is one more memory access, which worsens performance.
    // without the ptr it is on par with naive implementation.
    // pixel (*avgptr)(int, int, int, pixel *) = avg_mid;

    for (i = 0; i < dim; i++) {
        
        isEdge += ( (i == 0) || (i == (dim-1)))? 1 : 0;
    
	for (j = 0; j < dim; j++) {
	
	    // this should not run if above already selected
	    isEdge += ( (j == 0) || (j == (dim-1)))? 1 : 0;
	    // avgptr = ( isEdge > 0)? avg_edge : avg_mid;
	    // dst[RIDX(i, j, dim)] = avgptr(dim, i, j, src);
	   if (isEdge) dst[RIDX(i, j, dim)] = avg_edge(dim, i, j, src);
	   else dst[RIDX(i, j, dim)] = avg_mid(dim, i, j, src);
	    
	    
	}
	    
	    isEdge = 0;
    }
}
*/

// smooth with separate looping nests for edge and mid cases
// this approach seems to work. Mean = 11+. For all other cases Mean=3.8
// next frontier is unpacking code in smooth_3, starting with the 
// most common case of the 3x3 in the inner loop of nonedge pixels.
// when used with completely unpacked avg fcns, Mean=25+
char smooth_descr_3[] = "smooth_3: separate loops for mid, edge cases";
void smooth_3(int dim, pixel *src, pixel *dst) 
{
    int i, j;
	   
    // top row edge cases
    dst[RIDX(0, 0, dim)] = avg_edge(dim, 0, 0, src);
    for (j = 1; j < dim-1; j++) {
	    dst[RIDX(0, j, dim)] = avg_top(dim, 0, j, src); 
    }   
    dst[RIDX(0, dim-1, dim)] = avg_edge(dim, 0, dim-1, src); 
    
    // mid cases and first, last column elements
    // first and last elements in row to be averaged with edge
    for (i = 1; i < (dim-1); i++) {
    	dst[RIDX(i, 0, dim)] = avg_left(dim, i, 0, src); // left
	for (j = 1; j < (dim-1); j++) {
	    dst[RIDX(i, j, dim)] = avg_mid_unpacked(dim, i, j, src); 
	} 
	dst[RIDX(i, (dim-1), dim)] = avg_right(dim, i, (dim-1), src); // right  
    }
	   
	   
    // bottom row edge cases
    dst[RIDX((dim-1), 0, dim)] = avg_edge(dim, (dim-1), 0, src);
    for (j = 1; j < dim-1; j++)
	    dst[RIDX((dim-1), j, dim)] = avg_bottom(dim, (dim-1), j, src);
	
    dst[RIDX((dim-1), dim-1, dim)] = avg_edge(dim, (dim-1), dim-1, src);    
	        
}

// code in: https://github.com/Exely/CSAPP-Labs/blob/master/notes/perflab.md
// Note: a huge amount of effort leading to a Mean of ~29, which
// is only an improvement of some 10% on smooth_3. Code not shown here.
// He has a correct observation on the branch prediction penalty;
// it is large for small numbers of iterations
/*
char smooth_descr_4[] = "smooth_4: COPY FROM CHINESE STUDENT";
void smooth_4(int dim, pixel *src, pixel *dst)
{
}
*/

/*
 * smooth - Your current working version of smooth. 
 * IMPORTANT: This is the version you will be graded on
 */
char smooth_descr[] = "smooth: Current working version";
void smooth(int dim, pixel *src, pixel *dst) 
{
    smooth_3(dim, src, dst);
}


/********************************************************************* 
 * register_smooth_functions - Register all of your different versions
 *     of the smooth kernel with the driver by calling the
 *     add_smooth_function() for each test function.  When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.  
 *********************************************************************/

void register_smooth_functions() {
    add_smooth_function(&smooth, smooth_descr);
    // add_smooth_function(&naive_smooth, naive_smooth_descr);
    /* ... Register additional test functions here */
    // add_smooth_function(&smooth_1, smooth_descr_1);
    // add_smooth_function(&smooth_2, smooth_descr_2);
    // add_smooth_function(&smooth_3, smooth_descr_3); 
    // add_smooth_function(&smooth_4, smooth_descr_4);       
    
    
    
}

