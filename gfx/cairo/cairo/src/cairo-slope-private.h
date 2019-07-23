




































#ifndef _CAIRO_SLOPE_PRIVATE_H
#define _CAIRO_SLOPE_PRIVATE_H

#include "cairo-types-private.h"
#include "cairo-fixed-private.h"

static inline void
_cairo_slope_init (cairo_slope_t *slope,
		   const cairo_point_t *a,
		   const cairo_point_t *b)
{
    slope->dx = b->x - a->x;
    slope->dy = b->y - a->y;
}

static inline cairo_bool_t
_cairo_slope_equal (const cairo_slope_t *a, const cairo_slope_t *b)
{
    return _cairo_int64_eq (_cairo_int32x32_64_mul (a->dy, b->dx),
			    _cairo_int32x32_64_mul (b->dy, a->dx));
}

static inline cairo_bool_t
_cairo_slope_backwards (const cairo_slope_t *a, const cairo_slope_t *b)
{
    return _cairo_int64_negative (_cairo_int64_add (_cairo_int32x32_64_mul (a->dx, b->dx),
						    _cairo_int32x32_64_mul (a->dy, b->dy)));
}

cairo_private int
_cairo_slope_compare (const cairo_slope_t *a,
	              const cairo_slope_t *b) cairo_pure;


#endif 
