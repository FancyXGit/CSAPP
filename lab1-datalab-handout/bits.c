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
// 1
/*
 * bitXor - x^y using only ~ and &
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
int bitXor(int x, int y)
{
  return ~((~(x & (~y))) & (~((~x) & y)));
}
/*
 * tmin - return minimum two's complement integer
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void)
{
  return 1 << 31;
}
// 2
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 1
 */
int isTmax(int x)
{
  int y = x + 1;
  return !(~(x + y) | !y);
}
/*
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   where bits are numbered from 0 (least significant) to 31 (most significant)
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x)
{
  int a4 = (0xAA << 8) + 0xAA;
  int a8 = (a4 << 16) + a4;
  return !(~((x & a8) | (~a8)));
}
/*
 * negate - return -x
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x)
{
  return ~x + 1;
}
// 3
/*
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
int isAsciiDigit(int x)
{
  int is_prefix_3 = !((x >> 4) ^ 3);
  int between_0_9 = ((x + 6) >> 4) & 1;
  return is_prefix_3 & between_0_9;
}
/*
 * conditional - same as x ? y : z
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z)
{
  int all_x = (!!x) << 31 >> 31;
  return (all_x & y) | (~all_x & z);
}
/*
 * isLessOrEqual - if x <= y  then return 1, else return 0
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y)
{
  int neagtive_x = ~x + 1;
  int y_sub_x = y + neagtive_x;
  int top_sub = y_sub_x >> 31;
  // if top_sub = 1111... y - x < 0
  // if top_sub = 0000... y - x >= 0
  return !top_sub;
}
// 4
/*
 * logicalNeg - implement the ! operator, using all of
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4
 */
int logicalNeg(int x)
{
  return ~((x | (~x + 1)) >> 31) & 1;
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
int howManyBits(int x)
{
  // n bits can represent from -2^n to 2^n - 1
  int res = 0;
  // is_positive get 1111... when x >= 0 and 0000... when x < 0
  int is_positive = ~(x >> 31);
  // 1111 ... 1011 -> 1011 requires 4 bits
  // positived 0000 ... 0100 -> 0100 requires 4 bits
  // original positive number 0000 ... 0111 -> 0111 requires 4 bits
  // so negative can transform to positive with required bits stay the same
  int positived_x = (is_positive & x) + (~is_positive & (~x));
  // set default 16
  int curr_pos = 16;
  // stage 16 bit
  int bits_16 = positived_x >> curr_pos;
  // declaration for old C
  int bits_8;
  int if_bits_8;
  int bits_4;
  int if_bits_4;
  int bits_2;
  int if_bits_2;
  int bit_1;
  int if_bit_1;

  int is_zero;
  int is_minus_one;
  int special;

  // be 1111 ... if bits_16 contains 1
  int if_bits_16 = ~(!!bits_16) + 1;
  // if 16-31 contains 1
  // curr_pos add 8 (be 24)
  // check 24 - 31 (top_8 of top_bits_16)
  // res add 16
  // else
  // curr_pos sub 8 (be 8)
  // check 8 - 15 (top_8 of lower_bits_16)
  curr_pos = ((curr_pos + 8) & if_bits_16) + ((curr_pos + ~8 + 1) & (~if_bits_16));
  bits_8 = positived_x >> curr_pos & 255;
  res += if_bits_16 & 16;
  // stage 8 bit
  if_bits_8 = ~(!!bits_8) + 1;
  curr_pos = ((curr_pos + 4) & if_bits_8) + ((curr_pos + ~4 + 1) & (~if_bits_8));
  bits_4 = positived_x >> curr_pos & 15;
  res += if_bits_8 & 8;
  // stage 4 bit
  if_bits_4 = ~(!!bits_4) + 1;
  curr_pos = ((curr_pos + 2) & if_bits_4) + ((curr_pos + ~2 + 1) & (~if_bits_4));
  bits_2 = positived_x >> curr_pos & 3;
  res += if_bits_4 & 4;
  // stage 2 bit
  if_bits_2 = ~(!!bits_2) + 1;
  curr_pos = ((curr_pos + 1) & if_bits_2) + ((curr_pos + ~1 + 1) & (~if_bits_2));
  bit_1 = positived_x >> curr_pos & 1;
  res += if_bits_2 & 2;
  // stage 1 bit
  if_bit_1 = ~(!!bit_1) + 1;
  res += if_bit_1 & 1;
  // final
  // magic number
  res += 2;

  // handle 0 and -1
  is_zero = !x;
  is_minus_one = !~x;
  special = ~(is_zero | is_minus_one) + 1;
  res = (res & (~special)) + (1 & special);
  return res;
}
// float
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
unsigned floatScale2(unsigned uf)
{
  int s = uf >> 31 & 1;
  int e = uf >> 23 & 0xFF;
  int m = uf & 0x7FFFFF;
  // de-normalized (including 0 and -0)
  if (e == 0)
  {
    int m_res = m + m;
    int carry_flag = m_res >> 23;
    // turn to normalized
    if (carry_flag)
    {
      e = 1;
      m = m_res & 0x7FFFFF;
    }
    // not turn
    else
    {
      m = m_res;
    }
  }
  // normalized
  else if (e >= 1 && e <= 254)
  {
    // turn inf
    if (e == 254)
    {
      m = 0;
      e = 255;
    }
    // not turn
    else
    {
      e += 1;
    }
  }
  // inf and NaN stay the same
  // no worry

  return (s << 31) | (e << 23) | m;
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
int floatFloat2Int(unsigned uf)
{
  // 2^31 stand for 2147483648 (MAX_INT + 1)
  const int MAX_E = 0x9E;
  const int ONE_E = 0x7F;
  int s = uf >> 31 & 1;
  int e = uf >> 23 & 0xFF;
  int m = uf & 0x7FFFFF;
  int M = (1 << 23) | m;
  int res = 0;
  // deal with uf > MAX_INT or uf <= MIN_INT
  if (e >= MAX_E)
  {
    // return MIN_INT
    return 0x80000000u;
  }
  // deal with -1 < uf < 1
  if (e < ONE_E)
  {
      return 0;
  }
  // 127 + 23 = 150
  // 23 stands for 23 bits significand
  // 1.m * 2 ^ (e - 127) = (1 << 23 + m) >> 23 << (e - 127)
  // if e - 127 > 23 (e > 150)
  // 1.m * 2 ^ (e - 127) = M << (e - 150)
  if (e < 150) 
  {
    // get absolute value
    // right shift auto floor division
    res = M >> (150 - e);
  }
  else
  {
    // accuate
    res = M << (e - 150);
  }
  if (s == 0)
  {
    return res;
  }
  else
  {
    return -res;
  }
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
unsigned floatPower2(int x)
{
  const unsigned int uint_INF = 0x7F800000;
  if (x >= 128)
  {
    return uint_INF;
  }
  if (x < -149)
  {
    return 0u;
  }
  // normalized
  if (x >= -126)
  {
    return (x + 127) << 23;
  }
  // de-normalized
  else
  {
    return 1 << (x + 149);
  }
}
