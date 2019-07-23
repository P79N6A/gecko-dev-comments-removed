



































#ifndef CAIRO_FIXED_PRIVATE_H
#define CAIRO_FIXED_PRIVATE_H

#include "cairo-wideint-private.h"





typedef int32_t		cairo_fixed_16_16_t;
typedef cairo_int64_t	cairo_fixed_32_32_t;
typedef cairo_int64_t	cairo_fixed_48_16_t;
typedef cairo_int128_t	cairo_fixed_64_64_t;
typedef cairo_int128_t	cairo_fixed_96_32_t;





#define CAIRO_FIXED_BITS	32





#define CAIRO_FIXED_FRAC_BITS	16


typedef int32_t cairo_fixed_t;


typedef uint32_t cairo_fixed_unsigned_t;





#if (CAIRO_FIXED_BITS != 32)
# error CAIRO_FIXED_BITS must be 32, and the type must be a 32-bit type.
# error To remove this limitation, you will have to fix the tesselator.
#endif

#define CAIRO_FIXED_ONE        ((cairo_fixed_t)(1 << CAIRO_FIXED_FRAC_BITS))
#define CAIRO_FIXED_ONE_DOUBLE ((double)(1 << CAIRO_FIXED_FRAC_BITS))
#define CAIRO_FIXED_EPSILON    ((cairo_fixed_t)(1))

#define CAIRO_FIXED_FRAC_MASK  (((cairo_fixed_unsigned_t)(-1)) >> (CAIRO_FIXED_BITS - CAIRO_FIXED_FRAC_BITS))
#define CAIRO_FIXED_WHOLE_MASK (~CAIRO_FIXED_FRAC_MASK)

static inline cairo_fixed_t
_cairo_fixed_from_int (int i)
{
    return i << CAIRO_FIXED_FRAC_BITS;
}






































#define CAIRO_MAGIC_NUMBER_FIXED_16_16 (103079215104.0)

#if CAIRO_FIXED_BITS <= 32
#define CAIRO_MAGIC_NUMBER_FIXED ((1LL << (52 - CAIRO_FIXED_FRAC_BITS)) * 1.5)


static inline cairo_fixed_t
_cairo_fixed_from_double (double d)
{
    union {
        double d;
        int32_t i[2];
    } u;

    u.d = d + CAIRO_MAGIC_NUMBER_FIXED;
#ifdef FLOAT_WORDS_BIGENDIAN
    return u.i[1];
#else
    return u.i[0];
#endif
}

#else
# error Please define a magic number for your fixed point type!
# error See cairo-fixed-private.h for details.
#endif

static inline cairo_fixed_t
_cairo_fixed_from_26_6 (uint32_t i)
{
#if CAIRO_FIXED_FRAC_BITS > 6
    return i << (CAIRO_FIXED_FRAC_BITS - 6);
#else
    return i >> (6 - CAIRO_FIXED_FRAC_BITS);
#endif
}

static inline double
_cairo_fixed_to_double (cairo_fixed_t f)
{
    return ((double) f) / CAIRO_FIXED_ONE_DOUBLE;
}

static inline int
_cairo_fixed_is_integer (cairo_fixed_t f)
{
    return (f & CAIRO_FIXED_FRAC_MASK) == 0;
}

static inline int
_cairo_fixed_integer_part (cairo_fixed_t f)
{
    return f >> CAIRO_FIXED_FRAC_BITS;
}

static inline int
_cairo_fixed_integer_floor (cairo_fixed_t f)
{
    if (f >= 0)
        return f >> CAIRO_FIXED_FRAC_BITS;
    else
        return -((-f - 1) >> CAIRO_FIXED_FRAC_BITS) - 1;
}

static inline int
_cairo_fixed_integer_ceil (cairo_fixed_t f)
{
    if (f > 0)
	return ((f - 1)>>CAIRO_FIXED_FRAC_BITS) + 1;
    else
	return - (-f >> CAIRO_FIXED_FRAC_BITS);
}





static inline cairo_fixed_16_16_t
_cairo_fixed_to_16_16 (cairo_fixed_t f)
{
#if CAIRO_FIXED_FRAC_BITS > 16
    return f >> (CAIRO_FIXED_FRAC_BITS - 16);
#else
    return f << (16 - CAIRO_FIXED_FRAC_BITS);
#endif
}

static inline cairo_fixed_16_16_t
_cairo_fixed_16_16_from_double (double d)
{
    union {
        double d;
        int32_t i[2];
    } u;

    u.d = d + CAIRO_MAGIC_NUMBER_FIXED_16_16;
#ifdef FLOAT_WORDS_BIGENDIAN
    return u.i[1];
#else
    return u.i[0];
#endif
}

#if CAIRO_FIXED_BITS == 32

static inline cairo_fixed_t
_cairo_fixed_mul (cairo_fixed_t a, cairo_fixed_t b)
{
    cairo_int64_t temp = _cairo_int32x32_64_mul (a, b);
    return _cairo_int64_to_int32(_cairo_int64_rsl (temp, CAIRO_FIXED_FRAC_BITS));
}

#else
# error Please define multiplication and other operands for your fixed-point type size
#endif

#endif 
