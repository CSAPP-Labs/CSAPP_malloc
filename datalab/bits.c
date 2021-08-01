/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting if the shift amount
     is less than 0 or greater than 31.


EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implement floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants. You can use any arithmetic,
logical, or comparison operations on int or unsigned data.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operations (integer, logical,
     or comparison) that you are allowed to use for your implementation
     of the function.  The max operator count is checked by dlc.
     Note that assignment ('=') is not counted; you may use as many of
     these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
//1
/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
int bitXor(int x, int y) {

	/* BIG CONSTANTS 0XFFFFFFFF NOT ALLOWED */
	int yd = (~x) & y;
	int xd = (~y) & x;

	int result = ~((~yd) & (~xd));


  return result;
}
/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {

  return 1<<31;

}
//2
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 1
 */
int isTmax(int x) { // USES 7 OPERATORS

	// incrementing has a flip effect on -1 and Tmax
	int inc = x+1;

	// if all 0s, then either Tmax or -1
	int a = (x ^ (inc)) + 1;

	// distinguish between Tmax and -1
	int b = !(!(inc));

	// when both a and b satisfied, determined that it is Tmax
	int result = (!a) & b;

  return result;
}
/* 
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   where bits are numbered from 0 (least significant) to 31 (most significant)
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x) {

	
	int bias = 0xAA; // 10101010
	int mask = 0;

	// could be a bit cleaner but it works like this
	mask = (((((bias<<8) | 0xAA ) <<8 ) | 0xAA ) <<8 ) | 0xAA;


  return !(((x & mask) | (~mask)) + 1);
}
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   csCSLegal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
  return ~x + 1;
}
//3
/* 
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
int isAsciiDigit(int x) {

	// maybe more efficient to not mask bottom 2 bits and use the failure downstream as part of the result
	
	int masked = (0xFF & (0xC0 ^ x)) ;		// 0xFF to look at only last byte. 0xC0 for ASCII # template
	
	int upper = (masked + 16)>>8;           // 0x1 if in 0x30 range
	
	int lower = ((0xF & masked) + 6)>>4;    // 0x1 if out of range because the unshifted sum is 16; beyond 0 to 9

  return (upper & (!lower) & !(x>>8));		// !(x>>8) a crude way to check if anything larger than ASCII # template
}
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z) { // uses 8 operations

	int notx = !x;

	int zd = (((notx) << 31 ) >> 31 ) & z;

	int yd = (~0 + notx) & y;

  	return zd | yd;
}
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) { // uses 21 operators. maybe use logic from logNeg ?

    int offset = 1<<31;
    
    int xa = x + offset;
    int ya = y + offset;
    
    int diff = xa ^ ya; // if all 0s, same value, return true, !diff
    int ndiff = ~diff;
    int yd = ya & diff;
    
    // an attempt to split the addition of differences and ndiff into high and low. 
    // only need the overflow results from the addlow, to add to addhigh
    // only need the eventual overflow from addhigh. if more than 0, that should
    // indicate that y is the greater of the two.
    
    int mask_low = 0xFF | (0xFF<<8); // or mask_high via ~0<<16
    int yd_low = (mask_low & yd);
    int ndiff_low = ndiff & mask_low;
    

    // both yd_high and yd_low are added twice; this can be done by a doubling
    // effected by appropriate right-shifts, 15 not 16?
    int addlow = (ndiff_low + yd_low + yd_low)>>16;
    
    int yd_high = (yd>>16) & mask_low;

    int ndiff_high = (ndiff>>16) & mask_low;
    
    int addhigh = (ndiff_high + yd_high + yd_high + addlow)>>16;

    int result = (addhigh | !diff);

  return result;
}
//4
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x) { 
	// 5 ops

    int result = ((x | (~x+1))>>31) + 1;

  return result;
}
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x) { // how to do it with fewer than 36 operators?
    int result = 1;
    int temp = 0;
    
    // if msb = 1, converted to their additive inv.
    // maybe the msb itself can be a shifter. if 0, nothing happens,if 1, it flips x=x^(msb>>31)+(!!msb).    
    // and if the magnitude is present in x, shorten x

    // 16+
    int mask = (~0)<<15;
    x=(x^(x>>31)); // flips the bits if negative nr, // +(!!x_msb)

    temp = ((!!(mask&x))<<4);
    result = result + temp;
    x = x>>temp;
    
    // 8+
    mask = mask>>8;
    temp = ((!!(mask&x))<<3);
    result = result + temp;
    x = x>>temp;
    
    // 4+
    mask = mask>>4;
    temp = ((!!(mask&x))<<2);
    result = result + temp;
    x = x>>temp;
    
    // 2+
    mask = mask>>2;
    temp = ((!!(mask&x))<<1);
    result = result + temp;
    x = x>>temp;
    
    // 1+ is preset in result
    mask = mask>>1;
    temp = ((!!(mask&x)));
    result = result + temp;
    // x = x>>temp;


  return result;
}
//float
/* 
 * floatScale2 - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned floatScale2(unsigned uf) { // do with less than 15 ops??
    
    unsigned result = 0x0;

    // task allows arbitrary constants. that can take all shiftmasks out of ops (4 of them)
    unsigned msb = 1<<31;
    unsigned mask = 0xFE<<23; 
    unsigned exp_mask = 0xFF<<23;
    unsigned frac = uf & (~mask);
    unsigned s = msb & uf; // save the sign bit

    // -1^s * M * 2^E 

	// do frac addition only for denormalized. flows correctly into normalized at edge.
    result = uf + frac;

    // exp increment if exp nonzero. overflow to inf if large normalized nr (e=11..0)gets doubled
    if ((exp_mask & uf)) {result = uf + (1<<23);}

    // reinsert the bit if all fine. do it in less than 3 ops?
    result = (result & (~msb)) ^ s;

    // +-0 * 2 = 0 .. no need to handle it // if ((~uf + 1) == uf) { return uf;}
    // inf * 2 = inf; NaN* 2 = NaN // ((uf>>23) & 0xFF) == 0xFF checking exp
    // if frac nonzero, NaN; ignoring frac, this case can also handle NaN, as exp all 1s
    if ( exp_mask == (exp_mask & uf) ) {return uf;}

  return result;
}
/* 
 * floatFloat2Int - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int floatFloat2Int(unsigned uf) { // do it in less than 12 ops?

	// all values |x|<1 round to 0. their exp is smaller than 01..1. includes all denorm.
	// for exp values same or bigger, rounding starts

	int exp = 0xFF & (uf>>23);
	int bias = 127; // for single precision.
	int E = exp - bias; 
	unsigned frac = 0xFFFFFF & uf; // (1<<23) | ((~(0xFE<<23)) & uf); // impute the implied bit

	// frac right under the int sign bit as default?
	int result = frac<<7;	

	// inf. NaN. return 0x80000000u ; means anything bigger than an exponent of 30.
	if (E > 30) {return 0x80000000u;}

	// below 1 returns 0. 1 is s_01111111_0s
	if (E < 0) {return 0;}

	// when E>30 overflows into inf, and when E<0 round to 0. would large shifts do this?
	// values in range implicitly rounded towards 0 as LSB dropped
	result = result>>(30 - E); 

	// now if s=1, additive inverse. arithmetic right on unsigned does not extend the signbit.
	if (uf>>31) {result = ~result + 1; }


  return result;
}
/* 
 * floatPower2 - Return bit-level equivalent of the expression 2.0^x
 *   (2.0 raised to the power x) for any 32-bit integer x.
 *
 *   The unsigned value that is returned should have the identical bit
 *   representation as the single-precision floating-point number 2.0^x.
 *   If the result is too small to be represented as a denorm, return
 *   0. If too large, return +INF.
 * 
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. Also if, while 
 *   Max ops: 30 
 *   Rating: 4
 */
unsigned floatPower2(int x) { 

	// for some reason only passes ./btest if timeout limit is changed to 20.
	// all other tests are either instantaneous or take about 5 secs


	// unsigned one = 0x3F800000; // s_01111111_0s
	int exp = 0x7f;
	// unsigned result = 0;
	// unsigned fracshift = 1<<23;

	if (x > 127) {return 0x7F800000;} // is this the value +INF

	// if ((x == 0x80000000)) {return 0;}

	if (x < -126) {return 0;}


	// for 127 to -126 add to the exp
	// result = (exp + x)<<23; 


	// for -127 to -149 shift the frac right
	// if (x < -126) {return (fracshift>>(-x-126));}


    return (exp + x)<<23;
}
