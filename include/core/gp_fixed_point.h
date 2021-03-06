// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2009-2012 Cyril Hrubis <metan@ucw.cz>
 */

/*

  Macros for fixed point arithmetic.

  We use 8 bits for fractional part for coordinates and sizes for Anti Aliased
  primitives.

 */

#ifndef CORE_GP_FIXED_POINT_H
#define CORE_GP_FIXED_POINT_H

#include <stdint.h>

/*
 * Number of bits used for fractional part.
 */
#define GP_FP_FRAC_BITS 8

/*
 * One
 */
#define GP_FP_1 (1<<GP_FP_FRAC_BITS)

/*
 * One Half
 */
#define GP_FP_1_2 (1<<(GP_FP_FRAC_BITS - 1))

/*
 * Fraction part bitmask.
 */
#define GP_FP_FRAC_MASK ((1<<GP_FP_FRAC_BITS)-1)

/*
 * Integer part bitmask.
 */
#define GP_FP_INT_MASK (~GP_FP_FRAC_MASK)

/*
 * Addition.
 */
#define GP_FP_ADD(a, b) ((a)+(b))

/*
 * Substraction.
 */
#define GP_FP_SUB(a, b) ((a)-(b))

/*
 * Multiplication, may overflow.
 */
#define GP_FP_MUL(a, b) ((((a) * (b)) + GP_FP_1_2)>>GP_FP_FRAC_BITS)

/*
 * Division, may overflow
 */
#define GP_FP_DIV(a, b) ((((a)<<GP_FP_FRAC_BITS) + GP_FP_1_2) / (b))

/*
 * Floor.
 */
#define GP_FP_FLOOR(a) ((a) & GP_FP_INT_MASK)

/*
 * Floor with conversion to integer.
 */
#define GP_FP_TO_INT(a) ((a)>>GP_FP_FRAC_BITS)
#define GP_FP_FLOOR_TO_INT(a) GP_FP_TO_INT(a)

/*
 * Ceiling.
 */
#define GP_FP_CEIL(a) (((a) & GP_FP_INT_MASK) + ((a) & GP_FP_FRAC_MASK) ? GP_FP_1 : 0)

/*
 * Ceilling with conversion to integer.
 */
#define GP_FP_CEIL_TO_INT(a) (((a)>>GP_FP_FRAC_BITS) + !!(GP_FP_FRAC(a)))

/*
 * Rounding.
 */
#define GP_FP_ROUND(a) GP_FP_FLOOR((a) + GP_FP_1_2)

/*
 * Rounding with conversion to integer.
 */
#define GP_FP_ROUND_TO_INT(a) ((((a) + GP_FP_1_2))>>GP_FP_FRAC_BITS)

/*
 * Fractional part.
 */
#define GP_FP_FRAC(a) ((a) & GP_FP_FRAC_MASK)

/*
 * Reverse fractional part 1 - frac(x)
 */
#define GP_FP_RFRAC(a) (GP_FP_1 - GP_FP_FRAC(a))

/*
 * Returns an float.
 */
#define GP_FP_TO_FLOAT(a) (((float)(a))/((float)GP_FP_1))

/*
 * Returns fixed point from integer.
 */
#define GP_FP_FROM_INT(a) ((a)<<GP_FP_FRAC_BITS)

#endif /* CORE_GP_FIXED_POINT_H */
