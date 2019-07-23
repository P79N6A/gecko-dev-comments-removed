






































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


#define P1x (line->p1.x)
#define P1y (line->p1.y)
#define P2x (line->p2.x)
#define P2y (line->p2.y)
#define B1x (box->p1.x)
#define B1y (box->p1.y)
#define B2x (box->p2.x)
#define B2y (box->p2.y)











cairo_bool_t
_cairo_box_intersects_line_segment (cairo_box_t *box, cairo_line_t *line)
{
    cairo_fixed_t t1, t2, t3, t4;
    cairo_int64_t t1y, t2y, t3x, t4x;

    cairo_fixed_t xlen, ylen;

    if (_cairo_box_contains_point(box, &line->p1) ||
	_cairo_box_contains_point(box, &line->p2))
	return TRUE;

    xlen = P2x - P1x;
    ylen = P2y - P1y;

    if (xlen) {
	if (xlen > 0) {
	    t1 = B1x - P1x;
	    t2 = B2x - P1x;
	} else {
	    t1 = P1x - B2x;
	    t2 = P1x - B1x;
	    xlen = - xlen;
	}

	if ((t1 < 0 || t1 > xlen) &&
	    (t2 < 0 || t2 > xlen))
	    return FALSE;
    } else {
	
	if (P1x < B1x || P1x > B2x)
	    return FALSE;
    }

    if (ylen) {
	if (ylen > 0) {
	    t3 = B1y - P1y;
	    t4 = B2y - P1y;
	} else {
	    t3 = P1y - B2y;
	    t4 = P1y - B1y;
	    ylen = - ylen;
	}

	if ((t3 < 0 || t3 > ylen) &&
	    (t4 < 0 || t4 > ylen))
	    return FALSE;
    } else {
	
	if (P1y < B1y || P1y > B2y)
	    return FALSE;
    }

    
    if (P1x == P2x || P1y == P2y)
	return TRUE;

    
    t1y = _cairo_int32x32_64_mul (t1, ylen);
    t2y = _cairo_int32x32_64_mul (t2, ylen);
    t3x = _cairo_int32x32_64_mul (t3, xlen);
    t4x = _cairo_int32x32_64_mul (t4, xlen);

    if (_cairo_int64_lt(t1y, t4x) &&
	_cairo_int64_lt(t3x, t2y))
	return TRUE;

    return FALSE;
}

cairo_bool_t
_cairo_box_contains_point (cairo_box_t *box, cairo_point_t *point)
{
    if (point->x < box->p1.x || point->x > box->p2.x ||
	point->y < box->p1.y || point->y > box->p2.y)
	return FALSE;
    return TRUE;
}
