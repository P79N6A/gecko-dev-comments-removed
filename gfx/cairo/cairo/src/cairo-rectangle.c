





































#include "cairoint.h"















void
_cairo_box_round_to_rectangle (cairo_box_t *box, cairo_rectangle_int_t *rectangle)
{
    rectangle->x = _cairo_fixed_integer_floor (box->p1.x);
    rectangle->y = _cairo_fixed_integer_floor (box->p1.y);
    rectangle->width = _cairo_fixed_integer_ceil (box->p2.x) - rectangle->x;
    rectangle->height = _cairo_fixed_integer_ceil (box->p2.y) - rectangle->y;
}

void
_cairo_rectangle_intersect (cairo_rectangle_int_t *dest, cairo_rectangle_int_t *src)
{
    int x1, y1, x2, y2;

    x1 = MAX (dest->x, src->x);
    y1 = MAX (dest->y, src->y);
    x2 = MIN (dest->x + dest->width, src->x + src->width);
    y2 = MIN (dest->y + dest->height, src->y + src->height);

    if (x1 >= x2 || y1 >= y2) {
	dest->x = 0;
	dest->y = 0;
	dest->width = 0;
	dest->height = 0;
    } else {
	dest->x = x1;
	dest->y = y1;
	dest->width = x2 - x1;
	dest->height = y2 - y1;
    }
}
