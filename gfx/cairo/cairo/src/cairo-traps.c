




































#include "cairoint.h"



static cairo_status_t
_cairo_traps_grow_by (cairo_traps_t *traps, int additional);

static cairo_status_t
_cairo_traps_add_trap (cairo_traps_t *traps, cairo_fixed_t top, cairo_fixed_t bottom,
		       cairo_line_t *left, cairo_line_t *right);

static int
_compare_point_fixed_by_y (const void *av, const void *bv);

static int
_compare_cairo_edge_by_top (const void *av, const void *bv);

static int
_compare_cairo_edge_by_slope (const void *av, const void *bv);

static cairo_fixed_16_16_t
_compute_x (cairo_line_t *line, cairo_fixed_t y);

static int
_line_segs_intersect_ceil (cairo_line_t *left, cairo_line_t *right, cairo_fixed_t *y_ret);

void
_cairo_traps_init (cairo_traps_t *traps)
{
    traps->status = CAIRO_STATUS_SUCCESS;

    traps->num_traps = 0;

    traps->traps_size = 0;
    traps->traps = NULL;
    traps->extents.p1.x = traps->extents.p1.y = INT16_MAX << 16;
    traps->extents.p2.x = traps->extents.p2.y = INT16_MIN << 16;
}

void
_cairo_traps_fini (cairo_traps_t *traps)
{
    if (traps->traps_size) {
	free (traps->traps);
	traps->traps = NULL;
	traps->traps_size = 0;
	traps->num_traps = 0;
    }
}










cairo_status_t
_cairo_traps_init_box (cairo_traps_t *traps,
		       cairo_box_t   *box)
{
  _cairo_traps_init (traps);

  traps->status = _cairo_traps_grow_by (traps, 1);
  if (traps->status)
    return traps->status;

  traps->num_traps = 1;

  traps->traps[0].top = box->p1.y;
  traps->traps[0].bottom = box->p2.y;
  traps->traps[0].left.p1 = box->p1;
  traps->traps[0].left.p2.x = box->p1.x;
  traps->traps[0].left.p2.y = box->p2.y;
  traps->traps[0].right.p1.x = box->p2.x;
  traps->traps[0].right.p1.y = box->p1.y;
  traps->traps[0].right.p2 = box->p2;

  traps->extents = *box;

  return traps->status;
}

static cairo_status_t
_cairo_traps_add_trap (cairo_traps_t *traps, cairo_fixed_t top, cairo_fixed_t bottom,
		       cairo_line_t *left, cairo_line_t *right)
{
    cairo_trapezoid_t *trap;

    if (traps->status)
	return traps->status;

    if (top == bottom) {
	return CAIRO_STATUS_SUCCESS;
    }

    if (traps->num_traps >= traps->traps_size) {
	int inc = traps->traps_size ? traps->traps_size : 32;
	traps->status = _cairo_traps_grow_by (traps, inc);
	if (traps->status)
	    return traps->status;
    }

    trap = &traps->traps[traps->num_traps];
    trap->top = top;
    trap->bottom = bottom;
    trap->left = *left;
    trap->right = *right;

    if (top < traps->extents.p1.y)
	traps->extents.p1.y = top;
    if (bottom > traps->extents.p2.y)
	traps->extents.p2.y = bottom;
    







    if (left->p1.x < traps->extents.p1.x)
	traps->extents.p1.x = left->p1.x;
    if (left->p2.x < traps->extents.p1.x)
	traps->extents.p1.x = left->p2.x;

    if (right->p1.x > traps->extents.p2.x)
	traps->extents.p2.x = right->p1.x;
    if (right->p2.x > traps->extents.p2.x)
	traps->extents.p2.x = right->p2.x;

    traps->num_traps++;

    return traps->status;
}

cairo_status_t
_cairo_traps_add_trap_from_points (cairo_traps_t *traps, cairo_fixed_t top, cairo_fixed_t bottom,
				   cairo_point_t left_p1, cairo_point_t left_p2,
				   cairo_point_t right_p1, cairo_point_t right_p2)
{
    cairo_line_t left;
    cairo_line_t right;

    if (traps->status)
	return traps->status;

    left.p1 = left_p1;
    left.p2 = left_p2;

    right.p1 = right_p1;
    right.p2 = right_p2;

    return _cairo_traps_add_trap (traps, top, bottom, &left, &right);
}

static cairo_status_t
_cairo_traps_grow_by (cairo_traps_t *traps, int additional)
{
    cairo_trapezoid_t *new_traps;
    int old_size = traps->traps_size;
    int new_size = traps->num_traps + additional;

    if (traps->status)
	return traps->status;

    if (new_size <= traps->traps_size)
	return traps->status;

    traps->traps_size = new_size;
    new_traps = realloc (traps->traps, traps->traps_size * sizeof (cairo_trapezoid_t));

    if (new_traps == NULL) {
	traps->traps_size = old_size;
	traps->status = CAIRO_STATUS_NO_MEMORY;
	return traps->status;
    }

    traps->traps = new_traps;

    return traps->status;
}

static int
_compare_point_fixed_by_y (const void *av, const void *bv)
{
    const cairo_point_t	*a = av, *b = bv;

    int ret = a->y - b->y;
    if (ret == 0) {
	ret = a->x - b->x;
    }
    return ret;
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
#define FIXED_MUL(_a, _b) \
            (_cairo_int64_to_int32(_cairo_int64_rsl(_cairo_int32x32_64_mul((_a), (_b)), 16)))

            offset_traps[i].top = FIXED_MUL(src_traps[i].top + yoff, ysc);
            offset_traps[i].bottom = FIXED_MUL(src_traps[i].bottom + yoff, ysc);
            offset_traps[i].left.p1.x = FIXED_MUL(src_traps[i].left.p1.x + xoff, xsc);
            offset_traps[i].left.p1.y = FIXED_MUL(src_traps[i].left.p1.y + yoff, ysc);
            offset_traps[i].left.p2.x = FIXED_MUL(src_traps[i].left.p2.x + xoff, xsc);
            offset_traps[i].left.p2.y = FIXED_MUL(src_traps[i].left.p2.y + yoff, ysc);
            offset_traps[i].right.p1.x = FIXED_MUL(src_traps[i].right.p1.x + xoff, xsc);
            offset_traps[i].right.p1.y = FIXED_MUL(src_traps[i].right.p1.y + yoff, ysc);
            offset_traps[i].right.p2.x = FIXED_MUL(src_traps[i].right.p2.x + xoff, xsc);
            offset_traps[i].right.p2.y = FIXED_MUL(src_traps[i].right.p2.y + yoff, ysc);

#undef FIXED_MUL
        }
    }
}

cairo_status_t
_cairo_traps_tessellate_triangle (cairo_traps_t *traps, cairo_point_t t[3])
{
    cairo_line_t line;
    cairo_fixed_16_16_t intersect;
    cairo_point_t tsort[3];

    memcpy (tsort, t, 3 * sizeof (cairo_point_t));
    qsort (tsort, 3, sizeof (cairo_point_t), _compare_point_fixed_by_y);

    
    if (tsort[0].y == tsort[1].y) {
	if (tsort[0].x < tsort[1].x)
	    _cairo_traps_add_trap_from_points (traps,
					       tsort[1].y, tsort[2].y,
					       tsort[0], tsort[2],
					       tsort[1], tsort[2]);
	else
	    _cairo_traps_add_trap_from_points (traps,
					       tsort[1].y, tsort[2].y,
					       tsort[1], tsort[2],
					       tsort[0], tsort[2]);
	return traps->status;
    }

    line.p1 = tsort[0];
    line.p2 = tsort[1];

    intersect = _compute_x (&line, tsort[2].y);

    if (intersect < tsort[2].x) {
	_cairo_traps_add_trap_from_points (traps,
					   tsort[0].y, tsort[1].y,
					   tsort[0], tsort[1],
					   tsort[0], tsort[2]);
	_cairo_traps_add_trap_from_points (traps,
					   tsort[1].y, tsort[2].y,
					   tsort[1], tsort[2],
					   tsort[0], tsort[2]);
    } else {
	_cairo_traps_add_trap_from_points (traps,
					   tsort[0].y, tsort[1].y,
					   tsort[0], tsort[2],
					   tsort[0], tsort[1]);
	_cairo_traps_add_trap_from_points (traps,
					   tsort[1].y, tsort[2].y,
					   tsort[0], tsort[2],
					   tsort[1], tsort[2]);
    }

    return traps->status;
}

cairo_status_t
_cairo_traps_tessellate_convex_quad (cairo_traps_t *traps, cairo_point_t q[4])
{
    int a, b, c, d;
    int i;

    
    a = 0;
    for (i = 1; i < 4; i++)
	if (_compare_point_fixed_by_y (&q[i], &q[a]) < 0)
	    a = i;

    
    b = (a + 1) % 4;
    c = (a + 2) % 4;
    d = (a + 3) % 4;

    
    if (_compare_point_fixed_by_y (&q[d], &q[b]) < 0) {
	b = (a + 3) % 4;
	d = (a + 1) % 4;
    }

    






    if (q[c].y < q[d].y) {
	if (q[b].x < q[d].x) {
	    










	    _cairo_traps_add_trap_from_points (traps,
					       q[a].y, q[b].y,
					       q[a], q[b], q[a], q[d]);
	    _cairo_traps_add_trap_from_points (traps,
					       q[b].y, q[c].y,
					       q[b], q[c], q[a], q[d]);
	    _cairo_traps_add_trap_from_points (traps,
					       q[c].y, q[d].y,
					       q[c], q[d], q[a], q[d]);
	} else {
	    









	    _cairo_traps_add_trap_from_points (traps,
					       q[a].y, q[b].y,
					       q[a], q[d], q[a], q[b]);
	    _cairo_traps_add_trap_from_points (traps,
					       q[b].y, q[c].y,
					       q[a], q[d], q[b], q[c]);
	    _cairo_traps_add_trap_from_points (traps,
					       q[c].y, q[d].y,
					       q[a], q[d], q[c], q[d]);
	}
    } else {
	if (q[b].x < q[d].x) {
	    









	    _cairo_traps_add_trap_from_points (traps,
					       q[a].y, q[b].y,
					       q[a], q[b], q[a], q[d]);
	    _cairo_traps_add_trap_from_points (traps,
					       q[b].y, q[d].y,
					       q[b], q[c], q[a], q[d]);
	    _cairo_traps_add_trap_from_points (traps,
					       q[d].y, q[c].y,
					       q[b], q[c], q[d], q[c]);
	} else {
	    









	    _cairo_traps_add_trap_from_points (traps,
					       q[a].y, q[b].y,
					       q[a], q[d], q[a], q[b]);
	    _cairo_traps_add_trap_from_points (traps,
					       q[b].y, q[d].y,
					       q[a], q[d], q[b], q[c]);
	    _cairo_traps_add_trap_from_points (traps,
					       q[d].y, q[c].y,
					       q[d], q[c], q[b], q[c]);
	}
    }

    return traps->status;
}

static int
_compare_cairo_edge_by_top (const void *av, const void *bv)
{
    const cairo_edge_t *a = av, *b = bv;

    return a->edge.p1.y - b->edge.p1.y;
}






static int
_compare_cairo_edge_by_slope (const void *av, const void *bv)
{
    const cairo_edge_t *a = av, *b = bv;
    cairo_fixed_32_32_t d;

    cairo_fixed_48_16_t a_dx = a->edge.p2.x - a->edge.p1.x;
    cairo_fixed_48_16_t a_dy = a->edge.p2.y - a->edge.p1.y;
    cairo_fixed_48_16_t b_dx = b->edge.p2.x - b->edge.p1.x;
    cairo_fixed_48_16_t b_dy = b->edge.p2.y - b->edge.p1.y;

    d = b_dy * a_dx - a_dy * b_dx;

    if (d > 0)
	return 1;
    else if (d == 0)
	return 0;
    else
	return -1;
}

static int
_compare_cairo_edge_by_current_x_slope (const void *av, const void *bv)
{
    const cairo_edge_t *a = av, *b = bv;
    int ret;

    ret = a->current_x - b->current_x;
    if (ret == 0)
	ret = _compare_cairo_edge_by_slope (a, b);
    return ret;
}









































#define CAIRO_TRAPS_USE_NEW_INTERSECTION_CODE 0

#if CAIRO_TRAPS_USE_NEW_INTERSECTION_CODE
static const cairo_fixed_32_32_t
_det16_32 (cairo_fixed_16_16_t a,
	   cairo_fixed_16_16_t b,
	   cairo_fixed_16_16_t c,
	   cairo_fixed_16_16_t d)
{
    return _cairo_int64_sub (_cairo_int32x32_64_mul (a, d),
			     _cairo_int32x32_64_mul (b, c));
}

static const cairo_fixed_64_64_t
_det32_64 (cairo_fixed_32_32_t a,
	   cairo_fixed_32_32_t b,
	   cairo_fixed_32_32_t c,
	   cairo_fixed_32_32_t d)
{
    return _cairo_int128_sub (_cairo_int64x64_128_mul (a, d),
			      _cairo_int64x64_128_mul (b, c));
}

static const cairo_fixed_32_32_t
_fixed_16_16_to_fixed_32_32 (cairo_fixed_16_16_t a)
{
    return _cairo_int64_lsl (_cairo_int32_to_int64 (a), 16);
}

static int
_line_segs_intersect_ceil (cairo_line_t *l1, cairo_line_t *l2, cairo_fixed_t *y_intersection)
{
    cairo_fixed_16_16_t	dx1, dx2, dy1, dy2;
    cairo_fixed_32_32_t	den_det;
    cairo_fixed_32_32_t	l1_det, l2_det;
    cairo_fixed_64_64_t num_det;
    cairo_fixed_32_32_t	intersect_32_32;
    cairo_fixed_48_16_t	intersect_48_16;
    cairo_fixed_16_16_t	intersect_16_16;
    cairo_quorem128_t	qr;

    dx1 = l1->p1.x - l1->p2.x;
    dy1 = l1->p1.y - l1->p2.y;
    dx2 = l2->p1.x - l2->p2.x;
    dy2 = l2->p1.y - l2->p2.y;
    den_det = _det16_32 (dx1, dy1,
			 dx2, dy2);

    if (_cairo_int64_eq (den_det, _cairo_int32_to_int64(0)))
	return 0;

    l1_det = _det16_32 (l1->p1.x, l1->p1.y,
			l1->p2.x, l1->p2.y);
    l2_det = _det16_32 (l2->p1.x, l2->p1.y,
			l2->p2.x, l2->p2.y);

    num_det = _det32_64 (l1_det, _fixed_16_16_to_fixed_32_32 (dy1),
			 l2_det, _fixed_16_16_to_fixed_32_32 (dy2));

    




    qr = _cairo_int128_divrem (num_det, _cairo_int64_to_int128 (den_det));

    intersect_32_32 = _cairo_int128_to_int64 (qr.quo);

    






    if (_cairo_int128_ne (qr.rem, _cairo_int32_to_int128 (0)) &&
	(_cairo_int128_ge (num_det, _cairo_int32_to_int128 (0)) ==
	 _cairo_int64_ge (den_det, _cairo_int32_to_int64 (0))))
    {
	intersect_32_32 = _cairo_int64_add (intersect_32_32,
					    _cairo_int32_to_int64 (1));
    }

    




    intersect_32_32 = _cairo_int64_add (intersect_32_32,
					_cairo_int32_to_int64 ((1 << 16) - 1));
    intersect_48_16 = _cairo_int64_rsa (intersect_32_32, 16);

    


    intersect_16_16 = _cairo_int64_to_int32 (intersect_48_16);

    *y_intersection = intersect_16_16;

    return 1;
}
#endif 

static cairo_fixed_16_16_t
_compute_x (cairo_line_t *line, cairo_fixed_t y)
{
    cairo_fixed_16_16_t dx = line->p2.x - line->p1.x;
    cairo_fixed_32_32_t ex = (cairo_fixed_48_16_t) (y - line->p1.y) * (cairo_fixed_48_16_t) dx;
    cairo_fixed_16_16_t dy = line->p2.y - line->p1.y;

    return line->p1.x + (ex / dy);
}

#if ! CAIRO_TRAPS_USE_NEW_INTERSECTION_CODE
static double
_compute_inverse_slope (cairo_line_t *l)
{
    return (_cairo_fixed_to_double (l->p2.x - l->p1.x) /
	    _cairo_fixed_to_double (l->p2.y - l->p1.y));
}

static double
_compute_x_intercept (cairo_line_t *l, double inverse_slope)
{
    return _cairo_fixed_to_double (l->p1.x) - inverse_slope * _cairo_fixed_to_double (l->p1.y);
}

static int
_line_segs_intersect_ceil (cairo_line_t *l1, cairo_line_t *l2, cairo_fixed_t *y_ret)
{
    






    cairo_fixed_16_16_t y_intersect;
    double  m1 = _compute_inverse_slope (l1);
    double  b1 = _compute_x_intercept (l1, m1);
    double  m2 = _compute_inverse_slope (l2);
    double  b2 = _compute_x_intercept (l2, m2);

    if (m1 == m2)
	return 0;

    y_intersect = _cairo_fixed_from_double ((b2 - b1) / (m1 - m2));

    if (m1 < m2) {
	cairo_line_t *t;
	t = l1;
	l1 = l2;
	l2 = t;
    }

    



    if (_compute_x (l2, y_intersect) > _compute_x (l1, y_intersect))
	y_intersect++;
    













    if (_compute_x (l2, y_intersect) > _compute_x (l1, y_intersect))
	y_intersect++;
    


    if (_compute_x (l2, y_intersect) > _compute_x (l1, y_intersect))
	y_intersect++;
    
















    *y_ret = y_intersect;

    return 1;
}
#endif 




























cairo_status_t
_cairo_traps_tessellate_polygon (cairo_traps_t		*traps,
				 cairo_polygon_t	*poly,
				 cairo_fill_rule_t	fill_rule)
{
    int 		i, active, inactive;
    cairo_fixed_t	y, y_next, intersect;
    int			in_out, num_edges = poly->num_edges;
    cairo_edge_t	*edges = poly->edges;

    if (num_edges == 0)
	return CAIRO_STATUS_SUCCESS;

    qsort (edges, num_edges, sizeof (cairo_edge_t), _compare_cairo_edge_by_top);

    y = edges[0].edge.p1.y;
    active = 0;
    inactive = 0;
    while (active < num_edges) {
	while (inactive < num_edges && edges[inactive].edge.p1.y <= y)
	    inactive++;

	for (i = active; i < inactive; i++)
	    edges[i].current_x = _compute_x (&edges[i].edge, y);

	qsort (&edges[active], inactive - active,
	       sizeof (cairo_edge_t), _compare_cairo_edge_by_current_x_slope);

	
	y_next = edges[active].edge.p2.y;

	for (i = active; i < inactive; i++) {
	    if (edges[i].edge.p2.y < y_next)
		y_next = edges[i].edge.p2.y;
	    
	    if (i != inactive - 1 && edges[i].current_x != edges[i+1].current_x)
		if (_line_segs_intersect_ceil (&edges[i].edge, &edges[i+1].edge,
					       &intersect))
		    if (intersect > y && intersect < y_next)
			y_next = intersect;
	}
	
	if (inactive < num_edges && edges[inactive].edge.p1.y < y_next)
	    y_next = edges[inactive].edge.p1.y;

	
	in_out = 0;
	for (i = active; i < inactive - 1; i++) {
	    if (fill_rule == CAIRO_FILL_RULE_WINDING) {
		if (edges[i].clockWise)
		    in_out++;
		else
		    in_out--;
		if (in_out == 0)
		    continue;
	    } else {
		in_out++;
		if ((in_out & 1) == 0)
		    continue;
	    }
	    _cairo_traps_add_trap (traps, y, y_next, &edges[i].edge, &edges[i+1].edge);
	}

	
	for (i = active; i < inactive; i++) {
	    if (edges[i].edge.p2.y <= y_next) {
		memmove (&edges[active+1], &edges[active], (i - active) * sizeof (cairo_edge_t));
		active++;
	    }
	}

	y = y_next;
    }
    return traps->status;
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
_cairo_traps_contain (cairo_traps_t *traps, double x, double y)
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

void
_cairo_traps_extents (cairo_traps_t *traps, cairo_box_t *extents)
{
    *extents = traps->extents;
}














cairo_status_t
_cairo_traps_extract_region (cairo_traps_t      *traps,
			     pixman_region16_t **region)
{
    int i;

    for (i = 0; i < traps->num_traps; i++)
	if (!(traps->traps[i].left.p1.x == traps->traps[i].left.p2.x
	      && traps->traps[i].right.p1.x == traps->traps[i].right.p2.x
	      && _cairo_fixed_is_integer(traps->traps[i].top)
	      && _cairo_fixed_is_integer(traps->traps[i].bottom)
	      && _cairo_fixed_is_integer(traps->traps[i].left.p1.x)
	      && _cairo_fixed_is_integer(traps->traps[i].right.p1.x))) {
	    *region = NULL;
	    return CAIRO_STATUS_SUCCESS;
	}

    *region = pixman_region_create ();

    for (i = 0; i < traps->num_traps; i++) {
	int x = _cairo_fixed_integer_part(traps->traps[i].left.p1.x);
	int y = _cairo_fixed_integer_part(traps->traps[i].top);
	int width = _cairo_fixed_integer_part(traps->traps[i].right.p1.x) - x;
	int height = _cairo_fixed_integer_part(traps->traps[i].bottom) - y;

	



	if (width == 0 || height == 0)
	  continue;

	if (pixman_region_union_rect (*region, *region,
				      x, y, width, height) != PIXMAN_REGION_STATUS_SUCCESS) {
	    pixman_region_destroy (*region);
	    return CAIRO_STATUS_NO_MEMORY;
	}
    }

    return CAIRO_STATUS_SUCCESS;
}
