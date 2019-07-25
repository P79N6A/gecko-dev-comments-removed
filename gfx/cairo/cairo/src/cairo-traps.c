






































#include "cairoint.h"

#include "cairo-region-private.h"
#include "cairo-slope-private.h"



void
_cairo_traps_init (cairo_traps_t *traps)
{
    VG (VALGRIND_MAKE_MEM_UNDEFINED (traps, sizeof (cairo_traps_t)));

    traps->status = CAIRO_STATUS_SUCCESS;

    traps->maybe_region = 1;
    traps->is_rectilinear = 0;
    traps->is_rectangular = 0;

    traps->num_traps = 0;

    traps->traps_size = ARRAY_LENGTH (traps->traps_embedded);
    traps->traps = traps->traps_embedded;

    traps->num_limits = 0;
    traps->has_intersections = FALSE;
}

void
_cairo_traps_limit (cairo_traps_t	*traps,
		    const cairo_box_t	*limits,
		    int			 num_limits)
{
    traps->limits = limits;
    traps->num_limits = num_limits;
}

void
_cairo_traps_clear (cairo_traps_t *traps)
{
    traps->status = CAIRO_STATUS_SUCCESS;

    traps->maybe_region = 1;
    traps->is_rectilinear = 0;
    traps->is_rectangular = 0;

    traps->num_traps = 0;
    traps->has_intersections = FALSE;
}

void
_cairo_traps_fini (cairo_traps_t *traps)
{
    if (traps->traps != traps->traps_embedded)
	free (traps->traps);

    VG (VALGRIND_MAKE_MEM_NOACCESS (traps, sizeof (cairo_traps_t)));
}


static cairo_bool_t
_cairo_traps_grow (cairo_traps_t *traps)
{
    cairo_trapezoid_t *new_traps;
    int new_size = 4 * traps->traps_size;

    if (CAIRO_INJECT_FAULT ()) {
	traps->status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	return FALSE;
    }

    if (traps->traps == traps->traps_embedded) {
	new_traps = _cairo_malloc_ab (new_size, sizeof (cairo_trapezoid_t));
	if (new_traps != NULL)
	    memcpy (new_traps, traps->traps, sizeof (traps->traps_embedded));
    } else {
	new_traps = _cairo_realloc_ab (traps->traps,
	                               new_size, sizeof (cairo_trapezoid_t));
    }

    if (unlikely (new_traps == NULL)) {
	traps->status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	return FALSE;
    }

    traps->traps = new_traps;
    traps->traps_size = new_size;
    return TRUE;
}

void
_cairo_traps_add_trap (cairo_traps_t *traps,
		       cairo_fixed_t top, cairo_fixed_t bottom,
		       cairo_line_t *left, cairo_line_t *right)
{
    cairo_trapezoid_t *trap;

    if (unlikely (traps->num_traps == traps->traps_size)) {
	if (unlikely (! _cairo_traps_grow (traps)))
	    return;
    }

    trap = &traps->traps[traps->num_traps++];
    trap->top = top;
    trap->bottom = bottom;
    trap->left = *left;
    trap->right = *right;
}










cairo_status_t
_cairo_traps_init_boxes (cairo_traps_t	    *traps,
		         const cairo_box_t  *boxes,
			 int		     num_boxes)
{
    cairo_trapezoid_t *trap;

    _cairo_traps_init (traps);

    while (traps->traps_size < num_boxes) {
	if (unlikely (! _cairo_traps_grow (traps))) {
	    _cairo_traps_fini (traps);
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);
	}
    }

    traps->num_traps = num_boxes;
    traps->is_rectilinear = TRUE;
    traps->is_rectangular = TRUE;

    trap = &traps->traps[0];
    while (num_boxes--) {
	trap->top    = boxes->p1.y;
	trap->bottom = boxes->p2.y;

	trap->left.p1   = boxes->p1;
	trap->left.p2.x = boxes->p1.x;
	trap->left.p2.y = boxes->p2.y;

	trap->right.p1.x = boxes->p2.x;
	trap->right.p1.y = boxes->p1.y;
	trap->right.p2   = boxes->p2;

	if (traps->maybe_region) {
	    traps->maybe_region  = _cairo_fixed_is_integer (boxes->p1.x) &&
		                   _cairo_fixed_is_integer (boxes->p1.y) &&
		                   _cairo_fixed_is_integer (boxes->p2.x) &&
		                   _cairo_fixed_is_integer (boxes->p2.y);
	}

	trap++, boxes++;
    }

    return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_traps_tessellate_rectangle (cairo_traps_t *traps,
				   const cairo_point_t *top_left,
				   const cairo_point_t *bottom_right)
{
    cairo_line_t left;
    cairo_line_t right;
    cairo_fixed_t top, bottom;

    if (top_left->y == bottom_right->y)
	return CAIRO_STATUS_SUCCESS;

    if (top_left->x == bottom_right->x)
	return CAIRO_STATUS_SUCCESS;

     left.p1.x =  left.p2.x = top_left->x;
     left.p1.y = right.p1.y = top_left->y;
    right.p1.x = right.p2.x = bottom_right->x;
     left.p2.y = right.p2.y = bottom_right->y;

     top = top_left->y;
     bottom = bottom_right->y;

    if (traps->num_limits) {
	cairo_bool_t reversed;
	int n;

	
	reversed = top_left->x > bottom_right->x;
	if (reversed) {
	    right.p1.x = right.p2.x = top_left->x;
	    left.p1.x = left.p2.x = bottom_right->x;
	}

	for (n = 0; n < traps->num_limits; n++) {
	    const cairo_box_t *limits = &traps->limits[n];
	    cairo_line_t _left, _right;
	    cairo_fixed_t _top, _bottom;

	    if (top >= limits->p2.y)
		continue;
	    if (bottom <= limits->p1.y)
		continue;

	    

	    if (left.p1.x >= limits->p2.x)
		continue;
	    if (right.p1.x <= limits->p1.x)
		continue;

	    
	    _top = top;
	    if (_top < limits->p1.y)
		_top = limits->p1.y;

	    _bottom = bottom;
	    if (_bottom > limits->p2.y)
		_bottom = limits->p2.y;

	    if (_bottom <= _top)
		continue;

	    _left = left;
	    if (_left.p1.x < limits->p1.x) {
		_left.p1.x = limits->p1.x;
		_left.p1.y = limits->p1.y;
		_left.p2.x = limits->p1.x;
		_left.p2.y = limits->p2.y;
	    }

	    _right = right;
	    if (_right.p1.x > limits->p2.x) {
		_right.p1.x = limits->p2.x;
		_right.p1.y = limits->p1.y;
		_right.p2.x = limits->p2.x;
		_right.p2.y = limits->p2.y;
	    }

	    if (left.p1.x >= right.p1.x)
		continue;

	    if (reversed)
		_cairo_traps_add_trap (traps, _top, _bottom, &_right, &_left);
	    else
		_cairo_traps_add_trap (traps, _top, _bottom, &_left, &_right);
	}
    } else {
	_cairo_traps_add_trap (traps, top, bottom, &left, &right);
    }

    return traps->status;
}

void
_cairo_traps_translate (cairo_traps_t *traps, int x, int y)
{
    cairo_fixed_t xoff, yoff;
    cairo_trapezoid_t *t;
    int i;

    




    xoff = _cairo_fixed_from_int (x);
    yoff = _cairo_fixed_from_int (y);

    for (i = 0, t = traps->traps; i < traps->num_traps; i++, t++) {
	t->top += yoff;
	t->bottom += yoff;
	t->left.p1.x += xoff;
	t->left.p1.y += yoff;
	t->left.p2.x += xoff;
	t->left.p2.y += yoff;
	t->right.p1.x += xoff;
	t->right.p1.y += yoff;
	t->right.p2.x += xoff;
	t->right.p2.y += yoff;
    }
}

void
_cairo_trapezoid_array_translate_and_scale (cairo_trapezoid_t *offset_traps,
                                            cairo_trapezoid_t *src_traps,
                                            int num_traps,
                                            double tx, double ty,
                                            double sx, double sy)
{
    int i;
    cairo_fixed_t xoff = _cairo_fixed_from_double (tx);
    cairo_fixed_t yoff = _cairo_fixed_from_double (ty);

    if (sx == 1.0 && sy == 1.0) {
        for (i = 0; i < num_traps; i++) {
            offset_traps[i].top = src_traps[i].top + yoff;
            offset_traps[i].bottom = src_traps[i].bottom + yoff;
            offset_traps[i].left.p1.x = src_traps[i].left.p1.x + xoff;
            offset_traps[i].left.p1.y = src_traps[i].left.p1.y + yoff;
            offset_traps[i].left.p2.x = src_traps[i].left.p2.x + xoff;
            offset_traps[i].left.p2.y = src_traps[i].left.p2.y + yoff;
            offset_traps[i].right.p1.x = src_traps[i].right.p1.x + xoff;
            offset_traps[i].right.p1.y = src_traps[i].right.p1.y + yoff;
            offset_traps[i].right.p2.x = src_traps[i].right.p2.x + xoff;
            offset_traps[i].right.p2.y = src_traps[i].right.p2.y + yoff;
        }
    } else {
        cairo_fixed_t xsc = _cairo_fixed_from_double (sx);
        cairo_fixed_t ysc = _cairo_fixed_from_double (sy);

        for (i = 0; i < num_traps; i++) {
            offset_traps[i].top = _cairo_fixed_mul (src_traps[i].top + yoff, ysc);
            offset_traps[i].bottom = _cairo_fixed_mul (src_traps[i].bottom + yoff, ysc);
            offset_traps[i].left.p1.x = _cairo_fixed_mul (src_traps[i].left.p1.x + xoff, xsc);
            offset_traps[i].left.p1.y = _cairo_fixed_mul (src_traps[i].left.p1.y + yoff, ysc);
            offset_traps[i].left.p2.x = _cairo_fixed_mul (src_traps[i].left.p2.x + xoff, xsc);
            offset_traps[i].left.p2.y = _cairo_fixed_mul (src_traps[i].left.p2.y + yoff, ysc);
            offset_traps[i].right.p1.x = _cairo_fixed_mul (src_traps[i].right.p1.x + xoff, xsc);
            offset_traps[i].right.p1.y = _cairo_fixed_mul (src_traps[i].right.p1.y + yoff, ysc);
            offset_traps[i].right.p2.x = _cairo_fixed_mul (src_traps[i].right.p2.x + xoff, xsc);
            offset_traps[i].right.p2.y = _cairo_fixed_mul (src_traps[i].right.p2.y + yoff, ysc);
        }
    }
}

static cairo_bool_t
_cairo_trap_contains (cairo_trapezoid_t *t, cairo_point_t *pt)
{
    cairo_slope_t slope_left, slope_pt, slope_right;

    if (t->top > pt->y)
	return FALSE;
    if (t->bottom < pt->y)
	return FALSE;

    _cairo_slope_init (&slope_left, &t->left.p1, &t->left.p2);
    _cairo_slope_init (&slope_pt, &t->left.p1, pt);

    if (_cairo_slope_compare (&slope_left, &slope_pt) < 0)
	return FALSE;

    _cairo_slope_init (&slope_right, &t->right.p1, &t->right.p2);
    _cairo_slope_init (&slope_pt, &t->right.p1, pt);

    if (_cairo_slope_compare (&slope_pt, &slope_right) < 0)
	return FALSE;

    return TRUE;
}

cairo_bool_t
_cairo_traps_contain (const cairo_traps_t *traps,
		      double x, double y)
{
    int i;
    cairo_point_t point;

    point.x = _cairo_fixed_from_double (x);
    point.y = _cairo_fixed_from_double (y);

    for (i = 0; i < traps->num_traps; i++) {
	if (_cairo_trap_contains (&traps->traps[i], &point))
	    return TRUE;
    }

    return FALSE;
}

static cairo_fixed_t
_line_compute_intersection_x_for_y (const cairo_line_t *line,
				    cairo_fixed_t y)
{
    return _cairo_edge_compute_intersection_x_for_y (&line->p1, &line->p2, y);
}

void
_cairo_traps_extents (const cairo_traps_t *traps,
		      cairo_box_t *extents)
{
    int i;

    if (traps->num_traps == 0) {
	extents->p1.x = extents->p1.y = 0;
	extents->p2.x = extents->p2.y = 0;
	return;
    }

    extents->p1.x = extents->p1.y = INT32_MAX;
    extents->p2.x = extents->p2.y = INT32_MIN;

    for (i = 0; i < traps->num_traps; i++) {
	const cairo_trapezoid_t *trap =  &traps->traps[i];

	if (trap->top < extents->p1.y)
	    extents->p1.y = trap->top;
	if (trap->bottom > extents->p2.y)
	    extents->p2.y = trap->bottom;

	if (trap->left.p1.x < extents->p1.x) {
	    cairo_fixed_t x = trap->left.p1.x;
	    if (trap->top != trap->left.p1.y) {
		x = _line_compute_intersection_x_for_y (&trap->left,
							trap->top);
		if (x < extents->p1.x)
		    extents->p1.x = x;
	    } else
		extents->p1.x = x;
	}
	if (trap->left.p2.x < extents->p1.x) {
	    cairo_fixed_t x = trap->left.p2.x;
	    if (trap->bottom != trap->left.p2.y) {
		x = _line_compute_intersection_x_for_y (&trap->left,
							trap->bottom);
		if (x < extents->p1.x)
		    extents->p1.x = x;
	    } else
		extents->p1.x = x;
	}

	if (trap->right.p1.x > extents->p2.x) {
	    cairo_fixed_t x = trap->right.p1.x;
	    if (trap->top != trap->right.p1.y) {
		x = _line_compute_intersection_x_for_y (&trap->right,
							trap->top);
		if (x > extents->p2.x)
		    extents->p2.x = x;
	    } else
		extents->p2.x = x;
	}
	if (trap->right.p2.x > extents->p2.x) {
	    cairo_fixed_t x = trap->right.p2.x;
	    if (trap->bottom != trap->right.p2.y) {
		x = _line_compute_intersection_x_for_y (&trap->right,
							trap->bottom);
		if (x > extents->p2.x)
		    extents->p2.x = x;
	    } else
		extents->p2.x = x;
	}
    }
}
















cairo_int_status_t
_cairo_traps_extract_region (cairo_traps_t   *traps,
			     cairo_region_t **region)
{
    cairo_rectangle_int_t stack_rects[CAIRO_STACK_ARRAY_LENGTH (cairo_rectangle_int_t)];
    cairo_rectangle_int_t *rects = stack_rects;
    cairo_int_status_t status;
    int i, rect_count;

    
    if (! traps->maybe_region)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    for (i = 0; i < traps->num_traps; i++) {
	if (traps->traps[i].left.p1.x != traps->traps[i].left.p2.x   ||
	    traps->traps[i].right.p1.x != traps->traps[i].right.p2.x ||
	    ! _cairo_fixed_is_integer (traps->traps[i].top)          ||
	    ! _cairo_fixed_is_integer (traps->traps[i].bottom)       ||
	    ! _cairo_fixed_is_integer (traps->traps[i].left.p1.x)    ||
	    ! _cairo_fixed_is_integer (traps->traps[i].right.p1.x))
	{
	    traps->maybe_region = FALSE;
	    return CAIRO_INT_STATUS_UNSUPPORTED;
	}
    }

    if (traps->num_traps > ARRAY_LENGTH (stack_rects)) {
	rects = _cairo_malloc_ab (traps->num_traps, sizeof (cairo_rectangle_int_t));

	if (unlikely (rects == NULL))
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }

    rect_count = 0;
    for (i = 0; i < traps->num_traps; i++) {
	int x1 = _cairo_fixed_integer_part (traps->traps[i].left.p1.x);
	int y1 = _cairo_fixed_integer_part (traps->traps[i].top);
	int x2 = _cairo_fixed_integer_part (traps->traps[i].right.p1.x);
	int y2 = _cairo_fixed_integer_part (traps->traps[i].bottom);

	rects[rect_count].x = x1;
	rects[rect_count].y = y1;
	rects[rect_count].width = x2 - x1;
	rects[rect_count].height = y2 - y1;

	rect_count++;
    }

    *region = cairo_region_create_rectangles (rects, rect_count);
    status = (*region)->status;

    if (rects != stack_rects)
	free (rects);

    return status;
}


static void
_sanitize_trap (cairo_trapezoid_t *t)
{
    cairo_trapezoid_t s = *t;

#define FIX(lr, tb, p) \
    if (t->lr.p.y != t->tb) { \
        t->lr.p.x = s.lr.p2.x + _cairo_fixed_mul_div_floor (s.lr.p1.x - s.lr.p2.x, s.tb - s.lr.p2.y, s.lr.p1.y - s.lr.p2.y); \
        t->lr.p.y = s.tb; \
    }
    FIX (left,  top,    p1);
    FIX (left,  bottom, p2);
    FIX (right, top,    p1);
    FIX (right, bottom, p2);
}

cairo_private cairo_status_t
_cairo_traps_path (const cairo_traps_t *traps,
		   cairo_path_fixed_t  *path)
{
    int i;

    for (i = 0; i < traps->num_traps; i++) {
	cairo_status_t status;
	cairo_trapezoid_t trap = traps->traps[i];

	if (trap.top == trap.bottom)
	    continue;

	_sanitize_trap (&trap);

	status = _cairo_path_fixed_move_to (path, trap.left.p1.x, trap.top);
	if (unlikely (status)) return status;
	status = _cairo_path_fixed_line_to (path, trap.right.p1.x, trap.top);
	if (unlikely (status)) return status;
	status = _cairo_path_fixed_line_to (path, trap.right.p2.x, trap.bottom);
	if (unlikely (status)) return status;
	status = _cairo_path_fixed_line_to (path, trap.left.p2.x, trap.bottom);
	if (unlikely (status)) return status;
	status = _cairo_path_fixed_close_path (path);
	if (unlikely (status)) return status;
    }

    return CAIRO_STATUS_SUCCESS;
}
