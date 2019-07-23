



































#include "cairoint.h"

void
_cairo_slope_init (cairo_slope_t *slope, cairo_point_t *a, cairo_point_t *b)
{
    slope->dx = b->x - a->x;
    slope->dy = b->y - a->y;
}












int
_cairo_slope_compare (cairo_slope_t *a, cairo_slope_t *b)
{
    cairo_fixed_48_16_t diff;

    diff = ((cairo_fixed_48_16_t) a->dy * (cairo_fixed_48_16_t) b->dx
	    - (cairo_fixed_48_16_t) b->dy * (cairo_fixed_48_16_t) a->dx);

    if (diff > 0)
	return 1;
    if (diff < 0)
	return -1;

    if (a->dx == 0 && a->dy == 0)
	return 1;
    if (b->dx == 0 && b->dy ==0)
	return -1;

    return 0;
}












int
_cairo_slope_clockwise (cairo_slope_t *a, cairo_slope_t *b)
{
    return _cairo_slope_compare (a, b) < 0;
}

int
_cairo_slope_counter_clockwise (cairo_slope_t *a, cairo_slope_t *b)
{
    return ! _cairo_slope_clockwise (a, b);
}
