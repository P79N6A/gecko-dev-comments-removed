



































#include "cairoint.h"

cairo_fixed_t
_cairo_fixed_from_int (int i)
{
    return i << 16;
}




































#define CAIRO_MAGIC_NUMBER_FIXED_16_16 (103079215104.0)
cairo_fixed_t
_cairo_fixed_from_double (double d)
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

cairo_fixed_t
_cairo_fixed_from_26_6 (uint32_t i)
{
    return i << 10;
}

double
_cairo_fixed_to_double (cairo_fixed_t f)
{
    return ((double) f) / 65536.0;
}

int
_cairo_fixed_is_integer (cairo_fixed_t f)
{
    return (f & 0xFFFF) == 0;
}

int
_cairo_fixed_integer_part (cairo_fixed_t f)
{
    return f >> 16;
}

int
_cairo_fixed_integer_floor (cairo_fixed_t f)
{
    if (f >= 0)
	return f >> 16;
    else
	return -((-f - 1) >> 16) - 1;
}

int
_cairo_fixed_integer_ceil (cairo_fixed_t f)
{
    if (f > 0)
	return ((f - 1)>>16) + 1;
    else
	return - (-f >> 16);
}
