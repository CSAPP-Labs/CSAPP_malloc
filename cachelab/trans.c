/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 * cache parameters s 5, E 1, b 5; 32 cache sets, 1 line per set, 32 byte block in a line. Cache size 1KB.
 
 * SIDENOTE: "It is ok to implement separate code optimized for each 
 * matrix test case ; arrays illegal ; more than 12 local vars illegal"
 
 
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
 
void trans(int M, int N, int A[N][M], int B[M][N]);

 #define move(k) (B[j][i+k] = A[i+k][j]) // unused

char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{ 
 
  int tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;//, tmp8, tmp9, tmp10, tmp11;
  
  if (N == 32 || N == 67 ) {   
  // 32 x 32/4 = 256 ints can be held in the cache, which is a quarter of the 32x32 A matrix.
   
   // initial attempt used the move(k) macro, which is encoded to do a load, then a store.
   // this load/store alternation is sub-par, particularly for diagonal matrices, and had
   // misses around 350.
   
  // for nonsquare edge cases, only part of an array could be written. but arrays forbidden here.
  // so local variables ought be used. this means that 8x8 submatrix processing can be optimized.
  // but for 61x67, tackling both edge cases and the corner is too much effort.
  // 32x32 = 287 misses. 61x67 2008 misses (with arrays would be slightly below 2000)
  for (int a = 0; a < N; a+=8) {
     for (int b = 0; b < M; b+=8) {
        // Blocking: multiple reads from entire loaded cache line, then multiple writes, 
        // done for entirety of 32x32. but only for whole 8x8 submatrices in 61x67
        if ( ((a+7)<N) && (b+7)<M ){
		for (int c = a; (c < (a+8)) && (c < N); c++) {
		   // read whole row
		   tmp0 = A[c][b+0];tmp1 = A[c][b+1];tmp2 = A[c][b+2];tmp3 = A[c][b+3];tmp4 = A[c][b+4];tmp5 = A[c][b+5];tmp6 = A[c][b+6];tmp7 = A[c][b+7];
		   
		   // write whole row to columns
		   B[b+0][c]=tmp0; B[b+1][c]=tmp1;B[b+2][c]=tmp2; B[b+3][c]=tmp3;B[b+4][c]=tmp4; B[b+5][c]=tmp5;B[b+6][c]=tmp6; B[b+7][c]=tmp7; 
		}
        } else {
		// 8x8 submatrix, read/write alternate. resizes for edge cases in 
		// nonsquare matrices. also bounds checking for nonsquare matrices.
		// on its own: 32x32 = 343 misses. 61x67 = 2120 misses
		for (int c = a; (c < (a+8)) && (c < N); c++) {
		   for (int d = b; (d < (b+8)) && (d < M); d++) {
		       B[d][c] = A[c][d];
		   }           
		}         
        }
            
     }
  }

  
  }
  
  
  if (N == 64) {

  // Optimizing submatrix 8x8 processing away from diagonal: when applied to whole Amatrix, 1360 misses.
  // The 4 lines (each having 8 ints) read from A matrix make up the upper half of an 8x8 submatrix. 
  // Their writing extends to 8 half-rows in a downward vertical direction, of which the first 4
  // map to the same 4 cache sets(lines) as the last 4, causing thrashing. The thrashing for writing
  // is reduced by reading the first 4 lines entirely in 2 half-scans, one from 0, other from 4, 
  // and writing regularly: then reading the next 4 lines and writing them backwards to make use
  // of the lines already loaded for writing.
  
  // Optimizing for submatrices along the diagonal:

  // for further reduction of cache misses: mcginn7.github.io/2020/02/23/CSAPP-cachelab/

  for (int i = 0; i < N; i+=8) {
     for (int j = 0; j < M; j+=8) {        
        
        // non-interleaved code for 8x8 submatrix; on its own, 1363 misses
        // optimized for general submatrices. 
        // first 4 lines (rows) read from A matrix
        for (int l = 0; l < 4; l++) {        
        // read
        tmp0 = A[i][j+l]; tmp4 = A[i][j+4+l];
        tmp1 = A[i+1][j+l]; tmp5 = A[i+1][j+4+l];
        tmp2 = A[i+2][j+l]; tmp6 = A[i+2][j+4+l];
        tmp3 = A[i+3][j+l]; tmp7 = A[i+3][j+4+l];
        
        // write in regular order
        B[j+l][i] = tmp0; B[j+l][i+1] = tmp1; 
        B[j+l][i+2] = tmp2; B[j+l][i+3] = tmp3; 
        
        B[j+4+l][i] = tmp4; B[j+4+l][i+1] = tmp5; 
        B[j+4+l][i+2] = tmp6; B[j+4+l][i+3] = tmp7;
        
        }
        
        // last 4 lines read from A matrix. 
        // may be worth read/writing backwards to reduce write misses
        // !!or rather, writing last 4, then first 4 half-rows!!
        for (int l = 0; l < 4; l++) {
        // read
        tmp0 = A[i+4][j+l]; tmp4 = A[i+4][j+4+l];
        tmp1 = A[i+5][j+l]; tmp5 = A[i+5][j+4+l];
        tmp2 = A[i+6][j+l]; tmp6 = A[i+6][j+4+l];
        tmp3 = A[i+7][j+l]; tmp7 = A[i+7][j+4+l];
        
        // write, but in reverse to exploit the write-blocks left by
        // the writes of the first 4 lines (1363 misses)
        // when in normal order for both first and last 4 lines,
        // then 1587 misses.
        B[j+4+l][i+4] = tmp4; B[j+4+l][i+5] = tmp5; 
        B[j+4+l][i+6] = tmp6; B[j+4+l][i+7] = tmp7;  
              
        B[j+l][i+4] = tmp0; B[j+l][i+5] = tmp1; 
        B[j+l][i+6] = tmp2; B[j+l][i+7] = tmp3;  

        }
        
        // further optimization is for the matrix in the diagonal.
        // no good solution for it yet. solution in blog extensively uses 
        // a storage array of 8, and implies treating 4x4 sub-sub-matrices


        }
       
     }
  }  
  


}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

