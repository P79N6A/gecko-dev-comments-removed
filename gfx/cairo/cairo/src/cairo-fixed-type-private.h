



































#ifndef CAIRO_FIXED_TYPE_PRIVATE_H
#define CAIRO_FIXED_TYPE_PRIVATE_H

#include "cairo-wideint-type-private.h"





typedef int32_t		cairo_fixed_16_16_t;
typedef cairo_int64_t	cairo_fixed_32_32_t;
typedef cairo_int64_t	cairo_fixed_48_16_t;
typedef cairo_int128_t	cairo_fixed_64_64_t;
typedef cairo_int128_t	cairo_fixed_96_32_t;





#define CAIRO_FIXED_BITS	32





#define CAIRO_FIXED_FRAC_BITS	8


typedef int32_t cairo_fixed_t;


typedef uint32_t cairo_fixed_unsigned_t;

typedef struct _cairo_point {
    cairo_fixed_t x;
    cairo_fixed_t y;
} cairo_point_t;

#endif 
