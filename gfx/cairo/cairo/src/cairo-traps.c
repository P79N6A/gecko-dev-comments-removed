






































#include "cairoint.h"



static int
_compare_point_fixed_by_y (const void *av, const void *bv);

void
_cairo_traps_init (cairo_traps_t *traps)
{
    traps->status = CAIRO_STATUS_SUCCESS;

    traps->num_traps = 0;

    traps->traps_size = ARRAY_LENGTH (traps->traps_embedded);
    traps->traps = traps->traps_embedded;
    traps->extents.p1.x = traps->extents.p1.y = INT32_MAX;
    traps->extents.p2.x = traps->extents.p2.y = INT32_MIN;

    traps->has_limits = FALSE;
}

void
_cairo_traps_limit (cairo_traps_t	*traps,
		    cairo_box_t		*limits)
{
    traps->has_limits = TRUE;

    traps->limits = *limits;
}

cairo_bool_t
_cairo_traps_get_limit (cairo_traps_t *traps,
			cairo_box_t   *limits)
{
    *limits = traps->limits;
    return traps->has_limits;
}

void
_cairo_traps_clear (cairo_traps_t *traps)
{
    traps->status = CAIRO_STATUS_SUCCESS;

    traps->num_traps = 0;
    traps->extents.p1.x = traps->extents.p1.y = INT32_MAX;
    traps->extents.p2.x = traps->extents.p2.y = INT32_MIN;
}

void
_cairo_traps_fini (cairo_traps_t *traps)
{
    if (traps->traps != traps->traps_embedded)
	free (traps->traps);
}










void
_cairo_traps_init_box (cairo_traps_t *traps,
		       const cairo_box_t   *box)
{
    _cairo_traps_init (traps);

    assert (traps->traps_size >= 1);

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
}


static cairo_bool_t
_cairo_traps_grow (cairo_traps_t *traps)
{
    cairo_trapezoid_t *new_traps;
    int new_size = 2 * MAX (traps->traps_size, 16);

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

    












    if (traps->has_limits) {
	

	if (left->p1.x >= traps->limits.p2.x &&
	    left->p2.x >= traps->limits.p2.x)
	{
	    return;
	}

	if (right->p1.x <= traps->limits.p1.x &&
	    right->p2.x <= traps->limits.p1.x)
	{
	    return;
	}

	
	if (top > traps->limits.p2.y || bottom < traps->limits.p1.y)
	    return;

	





	if (top < traps->limits.p1.y)
	    top = traps->limits.p1.y;

	if (bottom > traps->limits.p2.y)
	    bottom = traps->limits.p2.y;

	if (left->p1.x <= traps->limits.p1.x &&
	    left->p2.x <= traps->limits.p1.x)
	{
	    left->p1.x = traps->limits.p1.x;
	    left->p2.x = traps->limits.p1.x;
	}

	if (right->p1.x >= traps->limits.p2.x &&
	    right->p2.x >= traps->limits.p2.x)
	{
	    right->p1.x = traps->limits.p2.x;
	    right->p2.x = traps->limits.p2.x;
	}
    }

    



    if (top >= bottom)
	return;
    
    if (right->p1.x <= left->p1.x && right->p1.y == left->p1.y &&
	right->p2.x <= left->p2.x && right->p2.y == left->p2.y)
	return;

    if (traps->num_traps == traps->traps_size) {
	if (! _cairo_traps_grow (traps))
	    return;
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




cairo_status_t
_cairo_traps_tessellate_triangle (cairo_traps_t *traps,
				  const cairo_point_t t[3])
{
    cairo_point_t quad[4];

    quad[0] = t[0];
    quad[1] = t[0];
    quad[2] = t[1];
    quad[3] = t[2];

    return _cairo_traps_tessellate_convex_quad (traps, quad);
}

cairo_status_t
_cairo_traps_tessellate_rectangle (cairo_traps_t *traps,
				   const cairo_point_t *top_left,
				   const cairo_point_t *bottom_right)
{
    cairo_line_t left;
    cairo_line_t right;

     left.p1.x =  left.p2.x = top_left->x;
     left.p1.y = right.p1.y = top_left->y;
    right.p1.x = right.p2.x = bottom_right->x;
     left.p2.y = right.p2.y = bottom_right->y;

    _cairo_traps_add_trap (traps, top_left->y, bottom_right->y, &left, &right);

    return traps->status;
}

cairo_status_t
_cairo_traps_tessellate_convex_quad (cairo_traps_t *traps,
				     const cairo_point_t q[4])
{
    int a, b, c, d;
    int i;
    cairo_slope_t ab, ad;
    cairo_bool_t b_left_of_d;
    cairo_line_t left;
    cairo_line_t right;

    
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

    

















    





    if (q[a].x == q[b].x && q[a].y == q[b].y)
	_cairo_slope_init (&ab, &q[a], &q[c]);
    else
	_cairo_slope_init (&ab, &q[a], &q[b]);

    _cairo_slope_init (&ad, &q[a], &q[d]);

    b_left_of_d = (_cairo_slope_compare (&ab, &ad) > 0);

    if (q[c].y <= q[d].y) {
	if (b_left_of_d) {
	    










	    left.p1  = q[a]; left.p2  = q[b];
	    right.p1 = q[a]; right.p2 = q[d];
	    _cairo_traps_add_trap (traps, q[a].y, q[b].y, &left, &right);
	    left.p1  = q[b]; left.p2  = q[c];
	    _cairo_traps_add_trap (traps, q[b].y, q[c].y, &left, &right);
	    left.p1  = q[c]; left.p2  = q[d];
	    _cairo_traps_add_trap (traps, q[c].y, q[d].y, &left, &right);
	} else {
	    









	    left.p1  = q[a]; left.p2  = q[d];
	    right.p1 = q[a]; right.p2 = q[b];
	    _cairo_traps_add_trap (traps, q[a].y, q[b].y, &left, &right);
	    right.p1 = q[b]; right.p2 = q[c];
	    _cairo_traps_add_trap (traps, q[b].y, q[c].y, &left, &right);
	    right.p1 = q[c]; right.p2 = q[d];
	    _cairo_traps_add_trap (traps, q[c].y, q[d].y, &left, &right);
	}
    } else {
	if (b_left_of_d) {
	    









	    left.p1  = q[a]; left.p2  = q[b];
	    right.p1 = q[a]; right.p2 = q[d];
	    _cairo_traps_add_trap (traps, q[a].y, q[b].y, &left, &right);
	    left.p1  = q[b]; left.p2  = q[c];
	    _cairo_traps_add_trap (traps, q[b].y, q[d].y, &left, &right);
	    right.p1 = q[d]; right.p2 = q[c];
	    _cairo_traps_add_trap (traps, q[d].y, q[c].y, &left, &right);
	} else {
	    









	    left.p1  = q[a]; left.p2  = q[d];
	    right.p1 = q[a]; right.p2 = q[b];
	    _cairo_traps_add_trap (traps, q[a].y, q[b].y, &left, &right);
	    right.p1 = q[b]; right.p2 = q[c];
	    _cairo_traps_add_trap (traps, q[b].y, q[d].y, &left, &right);
	    left.p1  = q[d]; left.p2  = q[c];
	    _cairo_traps_add_trap (traps, q[d].y, q[c].y, &left, &right);
	}
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

void
_cairo_traps_extents (const cairo_traps_t *traps,
		      cairo_box_t         *extents)
{
    if (traps->num_traps == 0) {
	extents->p1.x = extents->p1.y = _cairo_fixed_from_int (0);
	extents->p2.x = extents->p2.y = _cairo_fixed_from_int (0);
    } else {
	*extents = traps->extents;
	if (traps->has_limits) {
	    
	    if (extents->p1.x < traps->limits.p1.x)
		extents->p1.x = traps->limits.p1.x;
	    if (extents->p2.x > traps->limits.p2.x)
		extents->p2.x = traps->limits.p2.x;

	    if (extents->p1.y < traps->limits.p1.y)
		extents->p1.y = traps->limits.p1.y;
	    if (extents->p2.y > traps->limits.p2.y)
		extents->p2.y = traps->limits.p2.y;
	}
    }
}















cairo_int_status_t
_cairo_traps_extract_region (const cairo_traps_t  *traps,
			     cairo_region_t      **region)
{
    cairo_int_status_t status;
    cairo_region_t *r;
    int i;

    for (i = 0; i < traps->num_traps; i++) {
	if (traps->traps[i].left.p1.x != traps->traps[i].left.p2.x   ||
	    traps->traps[i].right.p1.x != traps->traps[i].right.p2.x ||
	    ! _cairo_fixed_is_integer (traps->traps[i].top)          ||
	    ! _cairo_fixed_is_integer (traps->traps[i].bottom)       ||
	    ! _cairo_fixed_is_integer (traps->traps[i].left.p1.x)    ||
	    ! _cairo_fixed_is_integer (traps->traps[i].right.p1.x))
	{
	    return CAIRO_INT_STATUS_UNSUPPORTED;
	}
    }

    r = cairo_region_create ();
    if (unlikely (r->status))
	return r->status;

    for (i = 0; i < traps->num_traps; i++) {
	cairo_rectangle_int_t rect;

	int x1 = _cairo_fixed_integer_part (traps->traps[i].left.p1.x);
	int y1 = _cairo_fixed_integer_part (traps->traps[i].top);
	int x2 = _cairo_fixed_integer_part (traps->traps[i].right.p1.x);
	int y2 = _cairo_fixed_integer_part (traps->traps[i].bottom);

	


	if (x1 == x2 || y1 == y2)
	    continue;

	rect.x = x1;
	rect.y = y1;
	rect.width = x2 - x1;
	rect.height = y2 - y1;

	status = cairo_region_union_rectangle (r, &rect);
	if (unlikely (status)) {
	    cairo_region_destroy (r);
	    return status;
	}
    }

    *region = r;
    return CAIRO_STATUS_SUCCESS;
}


static void
_sanitize_trap (cairo_trapezoid_t *t)
{
    cairo_trapezoid_t s = *t;

#define FIX(lr, tb, p) \
    if (t->lr.p.y != t->tb) { \
        t->lr.p.x = s.lr.p2.x + _cairo_fixed_mul_div (s.lr.p1.x - s.lr.p2.x, s.tb - s.lr.p2.y, s.lr.p1.y - s.lr.p2.y); \
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
