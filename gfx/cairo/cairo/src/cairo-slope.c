



































#include "cairoint.h"

void
_cairo_slope_init (cairo_slope_t *slope,
		   const cairo_point_t *a,
		   const cairo_point_t *b)
{
    slope->dx = b->x - a->x;
    slope->dy = b->y - a->y;
}



















int
_cairo_slope_compare (const cairo_slope_t *a, const cairo_slope_t *b)
{
    cairo_int64_t ady_bdx = _cairo_int32x32_64_mul (a->dy, b->dx);
    cairo_int64_t bdy_adx = _cairo_int32x32_64_mul (b->dy, a->dx);
    int cmp;

    cmp = _cairo_int64_cmp (ady_bdx, bdy_adx);
    if (cmp)
	return cmp;

    



    if (a->dx == 0 && a->dy == 0 && b->dx == 0 && b->dy ==0)
	return 0;
    if (a->dx == 0 && a->dy == 0)
	return 1;
    if (b->dx == 0 && b->dy ==0)
	return -1;

    








    if (((a->dx > 0) != (b->dx > 0)) ||
	((a->dy > 0) != (b->dy > 0)))
    {
	if (a->dx > 0 || (a->dx == 0 && a->dy > 0))
	    return +1;
	else
	    return -1;
    }

    
    return 0;
}
