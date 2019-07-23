




































#include "cairoint.h"
#include "cairo-path-fixed-private.h"

typedef struct cairo_stroker {
    cairo_stroke_style_t	*style;

    cairo_matrix_t *ctm;
    cairo_matrix_t *ctm_inverse;
    double tolerance;
    double ctm_determinant;
    cairo_bool_t ctm_det_positive;

    cairo_traps_t *traps;

    cairo_pen_t	  pen;

    cairo_point_t current_point;
    cairo_point_t first_point;

    cairo_bool_t has_initial_sub_path;

    cairo_bool_t has_current_face;
    cairo_stroke_face_t current_face;

    cairo_bool_t has_first_face;
    cairo_stroke_face_t first_face;

    cairo_bool_t dashed;
    unsigned int dash_index;
    cairo_bool_t dash_on;
    cairo_bool_t dash_starts_on;
    double dash_remain;

    cairo_bool_t has_bounds;
    cairo_box_t bounds;
} cairo_stroker_t;


static cairo_status_t
_cairo_stroker_init (cairo_stroker_t		*stroker,
		     cairo_stroke_style_t	*stroke_style,
		     cairo_matrix_t		*ctm,
		     cairo_matrix_t		*ctm_inverse,
		     double			 tolerance,
		     cairo_traps_t		*traps);

static void
_cairo_stroker_fini (cairo_stroker_t *stroker);

static cairo_status_t
_cairo_stroker_move_to (void *closure, cairo_point_t *point);

static cairo_status_t
_cairo_stroker_line_to (void *closure, cairo_point_t *point);

static cairo_status_t
_cairo_stroker_line_to_dashed (void *closure, cairo_point_t *point);

static cairo_status_t
_cairo_stroker_curve_to (void *closure,
			 cairo_point_t *b,
			 cairo_point_t *c,
			 cairo_point_t *d);

static cairo_status_t
_cairo_stroker_curve_to_dashed (void *closure,
				cairo_point_t *b,
				cairo_point_t *c,
				cairo_point_t *d);

static cairo_status_t
_cairo_stroker_close_path (void *closure);

static void
_translate_point (cairo_point_t *point, cairo_point_t *offset);

static int
_cairo_stroker_face_clockwise (cairo_stroke_face_t *in, cairo_stroke_face_t *out);

static cairo_status_t
_cairo_stroker_join (cairo_stroker_t *stroker, cairo_stroke_face_t *in, cairo_stroke_face_t *out);

static void
_cairo_stroker_start_dash (cairo_stroker_t *stroker)
{
    double offset;
    cairo_bool_t on = TRUE;
    unsigned int i = 0;

    offset = stroker->style->dash_offset;

    


    while (offset > 0.0 && offset >= stroker->style->dash[i]) {
	offset -= stroker->style->dash[i];
	on = !on;
	if (++i == stroker->style->num_dashes)
	    i = 0;
    }
    stroker->dashed = TRUE;
    stroker->dash_index = i;
    stroker->dash_on = stroker->dash_starts_on = on;
    stroker->dash_remain = stroker->style->dash[i] - offset;
}

static void
_cairo_stroker_step_dash (cairo_stroker_t *stroker, double step)
{
    stroker->dash_remain -= step;
    if (stroker->dash_remain <= 0) {
	stroker->dash_index++;
	if (stroker->dash_index == stroker->style->num_dashes)
	    stroker->dash_index = 0;
	stroker->dash_on = !stroker->dash_on;
	stroker->dash_remain = stroker->style->dash[stroker->dash_index];
    }
}

static cairo_status_t
_cairo_stroker_init (cairo_stroker_t		*stroker,
		     cairo_stroke_style_t	*stroke_style,
		     cairo_matrix_t		*ctm,
		     cairo_matrix_t		*ctm_inverse,
		     double			 tolerance,
		     cairo_traps_t		*traps)
{
    cairo_status_t status;
    stroker->style = stroke_style;
    stroker->ctm = ctm;
    stroker->ctm_inverse = ctm_inverse;
    stroker->tolerance = tolerance;
    stroker->traps = traps;

    stroker->ctm_determinant = _cairo_matrix_compute_determinant (stroker->ctm);
    stroker->ctm_det_positive = stroker->ctm_determinant >= 0.0;

    status = _cairo_pen_init (&stroker->pen,
		              stroke_style->line_width / 2.0,
			      tolerance, ctm);
    if (status)
	return status;

    stroker->has_current_face = FALSE;
    stroker->has_first_face = FALSE;
    stroker->has_initial_sub_path = FALSE;

    if (stroker->style->dash)
	_cairo_stroker_start_dash (stroker);
    else
	stroker->dashed = FALSE;

    stroker->has_bounds = _cairo_traps_get_limit (traps, &stroker->bounds);
    if (stroker->has_bounds) {
	



	double dx, dy;
	cairo_fixed_t fdx, fdy;

	_cairo_stroke_style_max_distance_from_path (stroker->style, stroker->ctm, &dx, &dy);

	fdx = _cairo_fixed_from_double (dx);
	fdy = _cairo_fixed_from_double (dy);

	stroker->bounds.p1.x -= fdx;
	stroker->bounds.p2.x += fdx;

	stroker->bounds.p1.y -= fdy;
	stroker->bounds.p2.y += fdy;
    }

    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_stroker_fini (cairo_stroker_t *stroker)
{
    _cairo_pen_fini (&stroker->pen);
}

static void
_translate_point (cairo_point_t *point, cairo_point_t *offset)
{
    point->x += offset->x;
    point->y += offset->y;
}

static int
_cairo_stroker_face_clockwise (cairo_stroke_face_t *in, cairo_stroke_face_t *out)
{
    cairo_slope_t in_slope, out_slope;

    _cairo_slope_init (&in_slope, &in->point, &in->cw);
    _cairo_slope_init (&out_slope, &out->point, &out->cw);

    return _cairo_slope_compare (&in_slope, &out_slope) < 0;
}







static int
_cairo_slope_compare_sgn (double dx1, double dy1, double dx2, double dy2)
{
    double  c = (dx1 * dy2 - dx2 * dy1);

    if (c > 0) return 1;
    if (c < 0) return -1;
    return 0;
}

static cairo_status_t
_cairo_stroker_join (cairo_stroker_t *stroker, cairo_stroke_face_t *in, cairo_stroke_face_t *out)
{
    int			clockwise = _cairo_stroker_face_clockwise (out, in);
    cairo_point_t	*inpt, *outpt;
    cairo_status_t status;

    if (in->cw.x == out->cw.x
	&& in->cw.y == out->cw.y
	&& in->ccw.x == out->ccw.x
	&& in->ccw.y == out->ccw.y)
    {
	return CAIRO_STATUS_SUCCESS;
    }

    if (clockwise) {
    	inpt = &in->ccw;
    	outpt = &out->ccw;
    } else {
    	inpt = &in->cw;
    	outpt = &out->cw;
    }

    switch (stroker->style->line_join) {
    case CAIRO_LINE_JOIN_ROUND: {
	int i;
	int start, step, stop;
	cairo_point_t tri[3];
	cairo_pen_t *pen = &stroker->pen;

	tri[0] = in->point;
	if (clockwise) {
	    _cairo_pen_find_active_ccw_vertex_index (pen, &in->dev_vector, &start);
	    step = -1;
	    _cairo_pen_find_active_ccw_vertex_index (pen, &out->dev_vector, &stop);
	} else {
	    _cairo_pen_find_active_cw_vertex_index (pen, &in->dev_vector, &start);
	    step = +1;
	    _cairo_pen_find_active_cw_vertex_index (pen, &out->dev_vector, &stop);
	}

	i = start;
	tri[1] = *inpt;
	while (i != stop) {
	    tri[2] = in->point;
	    _translate_point (&tri[2], &pen->vertices[i].point);
	    status = _cairo_traps_tessellate_triangle (stroker->traps, tri);
	    if (status)
		return status;
	    tri[1] = tri[2];
	    i += step;
	    if (i < 0)
		i = pen->num_vertices - 1;
	    if (i >= pen->num_vertices)
		i = 0;
	}

	tri[2] = *outpt;

	return _cairo_traps_tessellate_triangle (stroker->traps, tri);
    }
    case CAIRO_LINE_JOIN_MITER:
    default: {
	
	double	in_dot_out = ((-in->usr_vector.x * out->usr_vector.x)+
			      (-in->usr_vector.y * out->usr_vector.y));
	double	ml = stroker->style->miter_limit;

	
























































	if (2 <= ml * ml * (1 - in_dot_out))
	{
	    double		x1, y1, x2, y2;
	    double		mx, my;
	    double		dx1, dx2, dy1, dy2;
	    cairo_point_t	outer;
	    cairo_point_t	quad[4];
	    double		ix, iy;
	    double		fdx1, fdy1, fdx2, fdy2;
	    double		mdx, mdy;

	    





	    
	    x1 = _cairo_fixed_to_double (inpt->x);
	    y1 = _cairo_fixed_to_double (inpt->y);
	    dx1 = in->usr_vector.x;
	    dy1 = in->usr_vector.y;
	    cairo_matrix_transform_distance (stroker->ctm, &dx1, &dy1);

	    
	    x2 = _cairo_fixed_to_double (outpt->x);
	    y2 = _cairo_fixed_to_double (outpt->y);
	    dx2 = out->usr_vector.x;
	    dy2 = out->usr_vector.y;
	    cairo_matrix_transform_distance (stroker->ctm, &dx2, &dy2);

	    







	    my = (((x2 - x1) * dy1 * dy2 - y2 * dx2 * dy1 + y1 * dx1 * dy2) /
		  (dx1 * dy2 - dx2 * dy1));
	    if (fabs (dy1) >= fabs (dy2))
		mx = (my - y1) * dx1 / dy1 + x1;
	    else
		mx = (my - y2) * dx2 / dy2 + x2;

	    








	    ix = _cairo_fixed_to_double (in->point.x);
	    iy = _cairo_fixed_to_double (in->point.y);

	    
	    fdx1 = x1 - ix; fdy1 = y1 - iy;

	    
	    fdx2 = x2 - ix; fdy2 = y2 - iy;

	    
	    mdx = mx - ix; mdy = my - iy;

	    



	    if (_cairo_slope_compare_sgn (fdx1, fdy1, mdx, mdy) !=
		_cairo_slope_compare_sgn (fdx2, fdy2, mdx, mdy))
	    {
		


		outer.x = _cairo_fixed_from_double (mx);
		outer.y = _cairo_fixed_from_double (my);

		quad[0] = in->point;
		quad[1] = *inpt;
		quad[2] = outer;
		quad[3] = *outpt;

		return _cairo_traps_tessellate_convex_quad (stroker->traps, quad);
	    }
	}
	
    }
    case CAIRO_LINE_JOIN_BEVEL: {
	cairo_point_t tri[3];
	tri[0] = in->point;
	tri[1] = *inpt;
	tri[2] = *outpt;

	return _cairo_traps_tessellate_triangle (stroker->traps, tri);
    }
    }
}

static cairo_status_t
_cairo_stroker_add_cap (cairo_stroker_t *stroker, cairo_stroke_face_t *f)
{
    cairo_status_t	    status;

    if (stroker->style->line_cap == CAIRO_LINE_CAP_BUTT)
	return CAIRO_STATUS_SUCCESS;

    switch (stroker->style->line_cap) {
    case CAIRO_LINE_CAP_ROUND: {
	int i;
	int start, stop;
	cairo_slope_t slope;
	cairo_point_t tri[3];
	cairo_pen_t *pen = &stroker->pen;

	slope = f->dev_vector;
	_cairo_pen_find_active_cw_vertex_index (pen, &slope, &start);
	slope.dx = -slope.dx;
	slope.dy = -slope.dy;
	_cairo_pen_find_active_cw_vertex_index (pen, &slope, &stop);

	tri[0] = f->point;
	tri[1] = f->cw;
	for (i=start; i != stop; i = (i+1) % pen->num_vertices) {
	    tri[2] = f->point;
	    _translate_point (&tri[2], &pen->vertices[i].point);
	    status = _cairo_traps_tessellate_triangle (stroker->traps, tri);
	    if (status)
		return status;
	    tri[1] = tri[2];
	}
	tri[2] = f->ccw;

	return _cairo_traps_tessellate_triangle (stroker->traps, tri);
    }
    case CAIRO_LINE_CAP_SQUARE: {
	double dx, dy;
	cairo_slope_t	fvector;
	cairo_point_t	occw, ocw;
	cairo_polygon_t	polygon;

	dx = f->usr_vector.x;
	dy = f->usr_vector.y;
	dx *= stroker->style->line_width / 2.0;
	dy *= stroker->style->line_width / 2.0;
	cairo_matrix_transform_distance (stroker->ctm, &dx, &dy);
	fvector.dx = _cairo_fixed_from_double (dx);
	fvector.dy = _cairo_fixed_from_double (dy);
	occw.x = f->ccw.x + fvector.dx;
	occw.y = f->ccw.y + fvector.dy;
	ocw.x = f->cw.x + fvector.dx;
	ocw.y = f->cw.y + fvector.dy;

	_cairo_polygon_init (&polygon);
	_cairo_polygon_move_to (&polygon, &f->cw);
	_cairo_polygon_line_to (&polygon, &ocw);
	_cairo_polygon_line_to (&polygon, &occw);
	_cairo_polygon_line_to (&polygon, &f->ccw);
	_cairo_polygon_close (&polygon);
	status = _cairo_polygon_status (&polygon);

	if (status == CAIRO_STATUS_SUCCESS) {
	    status = _cairo_bentley_ottmann_tessellate_polygon (stroker->traps,
								&polygon,
								CAIRO_FILL_RULE_WINDING);
	}

	_cairo_polygon_fini (&polygon);

	return status;
    }
    case CAIRO_LINE_CAP_BUTT:
    default:
	return CAIRO_STATUS_SUCCESS;
    }
}

static cairo_status_t
_cairo_stroker_add_leading_cap (cairo_stroker_t     *stroker,
				cairo_stroke_face_t *face)
{
    cairo_stroke_face_t reversed;
    cairo_point_t t;

    reversed = *face;

    
    reversed.usr_vector.x = -reversed.usr_vector.x;
    reversed.usr_vector.y = -reversed.usr_vector.y;
    reversed.dev_vector.dx = -reversed.dev_vector.dx;
    reversed.dev_vector.dy = -reversed.dev_vector.dy;
    t = reversed.cw;
    reversed.cw = reversed.ccw;
    reversed.ccw = t;

    return _cairo_stroker_add_cap (stroker, &reversed);
}

static cairo_status_t
_cairo_stroker_add_trailing_cap (cairo_stroker_t     *stroker,
				 cairo_stroke_face_t *face)
{
    return _cairo_stroker_add_cap (stroker, face);
}

static inline cairo_bool_t
_compute_normalized_device_slope (double *dx, double *dy, cairo_matrix_t *ctm_inverse, double *mag_out)
{
    double dx0 = *dx, dy0 = *dy;
    double mag;

    cairo_matrix_transform_distance (ctm_inverse, &dx0, &dy0);

    if (dx0 == 0.0 && dy0 == 0.0) {
	if (mag_out)
	    *mag_out = 0.0;
	return FALSE;
    }

    if (dx0 == 0.0) {
	*dx = 0.0;
	if (dy0 > 0.0) {
	    mag = dy0;
	    *dy = 1.0;
	} else {
	    mag = -dy0;
	    *dy = -1.0;
	}
    } else if (dy0 == 0.0) {
	*dy = 0.0;
	if (dx0 > 0.0) {
	    mag = dx0;
	    *dx = 1.0;
	} else {
	    mag = -dx0;
	    *dx = -1.0;
	}
    } else {
	mag = sqrt (dx0 * dx0 + dy0 * dy0);
	*dx = dx0 / mag;
	*dy = dy0 / mag;
    }

    if (mag_out)
	*mag_out = mag;

    return TRUE;
}

static void
_compute_face (cairo_point_t *point, cairo_slope_t *dev_slope,
	       double slope_dx, double slope_dy,
	       cairo_stroker_t *stroker, cairo_stroke_face_t *face);

static cairo_status_t
_cairo_stroker_add_caps (cairo_stroker_t *stroker)
{
    cairo_status_t status;
    
    if (stroker->has_initial_sub_path
	&& !stroker->has_first_face
	&& !stroker->has_current_face
	&& stroker->style->line_cap == CAIRO_LINE_JOIN_ROUND)
    {
	
	double dx = 1.0, dy = 0.0;
	cairo_slope_t slope = { CAIRO_FIXED_ONE, 0 };
	cairo_stroke_face_t face;

	_compute_normalized_device_slope (&dx, &dy, stroker->ctm_inverse, NULL);

	

	_compute_face (&stroker->first_point, &slope, dx, dy, stroker, &face);

	status = _cairo_stroker_add_leading_cap (stroker, &face);
	if (status)
	    return status;
	status = _cairo_stroker_add_trailing_cap (stroker, &face);
	if (status)
	    return status;
    }

    if (stroker->has_first_face) {
	status = _cairo_stroker_add_leading_cap (stroker, &stroker->first_face);
	if (status)
	    return status;
    }

    if (stroker->has_current_face) {
	status = _cairo_stroker_add_trailing_cap (stroker, &stroker->current_face);
	if (status)
	    return status;
    }

    return CAIRO_STATUS_SUCCESS;
}

static void
_compute_face (cairo_point_t *point, cairo_slope_t *dev_slope,
	       double slope_dx, double slope_dy,
	       cairo_stroker_t *stroker, cairo_stroke_face_t *face)
{
    double face_dx, face_dy;
    cairo_point_t offset_ccw, offset_cw;

    






    if (stroker->ctm_det_positive)
    {
	face_dx = - slope_dy * (stroker->style->line_width / 2.0);
	face_dy = slope_dx * (stroker->style->line_width / 2.0);
    }
    else
    {
	face_dx = slope_dy * (stroker->style->line_width / 2.0);
	face_dy = - slope_dx * (stroker->style->line_width / 2.0);
    }

    
    cairo_matrix_transform_distance (stroker->ctm, &face_dx, &face_dy);

    offset_ccw.x = _cairo_fixed_from_double (face_dx);
    offset_ccw.y = _cairo_fixed_from_double (face_dy);
    offset_cw.x = -offset_ccw.x;
    offset_cw.y = -offset_ccw.y;

    face->ccw = *point;
    _translate_point (&face->ccw, &offset_ccw);

    face->point = *point;

    face->cw = *point;
    _translate_point (&face->cw, &offset_cw);

    face->usr_vector.x = slope_dx;
    face->usr_vector.y = slope_dy;

    face->dev_vector = *dev_slope;
}

static cairo_status_t
_cairo_stroker_add_sub_edge (cairo_stroker_t *stroker, cairo_point_t *p1, cairo_point_t *p2,
			     cairo_slope_t *dev_slope, double slope_dx, double slope_dy,
			     cairo_stroke_face_t *start, cairo_stroke_face_t *end)
{
    cairo_point_t rectangle[4];

    _compute_face (p1, dev_slope, slope_dx, slope_dy, stroker, start);

    


    _compute_face (p2, dev_slope, slope_dx, slope_dy, stroker, end);

    if (p1->x == p2->x && p1->y == p2->y)
	return CAIRO_STATUS_SUCCESS;

    rectangle[0] = start->cw;
    rectangle[1] = start->ccw;
    rectangle[2] = end->ccw;
    rectangle[3] = end->cw;

    return _cairo_traps_tessellate_convex_quad (stroker->traps, rectangle);
}

static cairo_status_t
_cairo_stroker_move_to (void *closure, cairo_point_t *point)
{
    cairo_status_t status;
    cairo_stroker_t *stroker = closure;

    
    status = _cairo_stroker_add_caps (stroker);
    if (status)
	return status;

    stroker->first_point = *point;
    stroker->current_point = *point;

    stroker->has_first_face = FALSE;
    stroker->has_current_face = FALSE;
    stroker->has_initial_sub_path = FALSE;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_stroker_move_to_dashed (void *closure, cairo_point_t *point)
{
    
    cairo_stroker_t *stroker = closure;
    _cairo_stroker_start_dash (stroker);

    return _cairo_stroker_move_to (closure, point);
}

static cairo_status_t
_cairo_stroker_line_to (void *closure, cairo_point_t *point)
{
    cairo_status_t status;
    cairo_stroker_t *stroker = closure;
    cairo_stroke_face_t start, end;
    cairo_point_t *p1 = &stroker->current_point;
    cairo_point_t *p2 = point;
    cairo_slope_t dev_slope;
    double slope_dx, slope_dy;

    stroker->has_initial_sub_path = TRUE;

    if (p1->x == p2->x && p1->y == p2->y)
	return CAIRO_STATUS_SUCCESS;

    _cairo_slope_init (&dev_slope, p1, p2);
    slope_dx = _cairo_fixed_to_double (p2->x - p1->x);
    slope_dy = _cairo_fixed_to_double (p2->y - p1->y);
    _compute_normalized_device_slope (&slope_dx, &slope_dy, stroker->ctm_inverse, NULL);

    status = _cairo_stroker_add_sub_edge (stroker, p1, p2, &dev_slope, slope_dx, slope_dy, &start, &end);
    if (status)
	return status;

    if (stroker->has_current_face) {
	
	status = _cairo_stroker_join (stroker, &stroker->current_face, &start);
	if (status)
	    return status;
    } else if (!stroker->has_first_face) {
	
	stroker->first_face = start;
	stroker->has_first_face = TRUE;
    }
    stroker->current_face = end;
    stroker->has_current_face = TRUE;

    stroker->current_point = *point;

    return CAIRO_STATUS_SUCCESS;
}




static cairo_status_t
_cairo_stroker_line_to_dashed (void *closure, cairo_point_t *point)
{
    cairo_stroker_t *stroker = closure;
    double mag, remain, step_length = 0;
    double slope_dx, slope_dy;
    double dx2, dy2;
    cairo_stroke_face_t sub_start, sub_end;
    cairo_point_t *p1 = &stroker->current_point;
    cairo_point_t *p2 = point;
    cairo_slope_t dev_slope;
    cairo_line_t segment;
    cairo_bool_t fully_in_bounds;
    cairo_status_t status;

    stroker->has_initial_sub_path = stroker->dash_starts_on;

    if (p1->x == p2->x && p1->y == p2->y)
	return CAIRO_STATUS_SUCCESS;

    fully_in_bounds = TRUE;
    if (stroker->has_bounds &&
	(! _cairo_box_contains_point (&stroker->bounds, p1) ||
	 ! _cairo_box_contains_point (&stroker->bounds, p2)))
    {
	fully_in_bounds = FALSE;
    }

    _cairo_slope_init (&dev_slope, p1, p2);

    slope_dx = _cairo_fixed_to_double (p2->x - p1->x);
    slope_dy = _cairo_fixed_to_double (p2->y - p1->y);

    if (! _compute_normalized_device_slope (&slope_dx, &slope_dy,
					   stroker->ctm_inverse, &mag))
    {
	return CAIRO_STATUS_SUCCESS;
    }

    remain = mag;
    segment.p1 = *p1;
    while (remain) {
	step_length = MIN (stroker->dash_remain, remain);
	remain -= step_length;
	dx2 = slope_dx * (mag - remain);
	dy2 = slope_dy * (mag - remain);
	cairo_matrix_transform_distance (stroker->ctm, &dx2, &dy2);
	segment.p2.x = _cairo_fixed_from_double (dx2) + p1->x;
	segment.p2.y = _cairo_fixed_from_double (dy2) + p1->y;

	if (stroker->dash_on &&
	    (fully_in_bounds ||
	     (! stroker->has_first_face && stroker->dash_starts_on) ||
	     _cairo_box_intersects_line_segment (&stroker->bounds, &segment)))
	{
	    status = _cairo_stroker_add_sub_edge (stroker,
						  &segment.p1, &segment.p2,
						  &dev_slope,
						  slope_dx, slope_dy,
						  &sub_start, &sub_end);
	    if (status)
		return status;

	    if (stroker->has_current_face) {
		
		status = _cairo_stroker_join (stroker,
					      &stroker->current_face,
					      &sub_start);
		if (status)
		    return status;

		stroker->has_current_face = FALSE;
	    } else if (! stroker->has_first_face && stroker->dash_starts_on) {
		
		stroker->first_face = sub_start;
		stroker->has_first_face = TRUE;
	    } else {
		
		status = _cairo_stroker_add_leading_cap (stroker, &sub_start);
		if (status)
		    return status;
	    }

	    if (remain) {
		
		status = _cairo_stroker_add_trailing_cap (stroker, &sub_end);
		if (status)
		    return status;
	    } else {
		stroker->current_face = sub_end;
		stroker->has_current_face = TRUE;
	    }
	} else {
	    if (stroker->has_current_face) {
		
		status = _cairo_stroker_add_trailing_cap (stroker,
							  &stroker->current_face);
		if (status)
		    return status;

		stroker->has_current_face = FALSE;
	    }
	}

	_cairo_stroker_step_dash (stroker, step_length);
	segment.p1 = segment.p2;
    }

    if (stroker->dash_on && ! stroker->has_current_face) {
	








	_compute_face (point, &dev_slope,
		       slope_dx, slope_dy,
		       stroker,
		       &stroker->current_face);

	status = _cairo_stroker_add_leading_cap (stroker,
						 &stroker->current_face);
	if (status)
	    return status;

	stroker->has_current_face = TRUE;
    }

    stroker->current_point = *point;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_stroker_curve_to (void *closure,
			 cairo_point_t *b,
			 cairo_point_t *c,
			 cairo_point_t *d)
{
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    cairo_stroker_t *stroker = closure;
    cairo_spline_t spline;
    cairo_pen_t pen;
    cairo_stroke_face_t start, end;
    cairo_point_t extra_points[4];
    cairo_point_t *a = &stroker->current_point;
    double initial_slope_dx, initial_slope_dy;
    double final_slope_dx, final_slope_dy;

    status = _cairo_spline_init (&spline, a, b, c, d);
    if (status == CAIRO_INT_STATUS_DEGENERATE)
	return _cairo_stroker_line_to (closure, d);

    status = _cairo_pen_init_copy (&pen, &stroker->pen);
    if (status)
	goto CLEANUP_SPLINE;

    initial_slope_dx = _cairo_fixed_to_double (spline.initial_slope.dx);
    initial_slope_dy = _cairo_fixed_to_double (spline.initial_slope.dy);
    final_slope_dx = _cairo_fixed_to_double (spline.final_slope.dx);
    final_slope_dy = _cairo_fixed_to_double (spline.final_slope.dy);

    if (_compute_normalized_device_slope (&initial_slope_dx, &initial_slope_dy, stroker->ctm_inverse, NULL))
	_compute_face (a, &spline.initial_slope, initial_slope_dx, initial_slope_dy, stroker, &start);

    if (_compute_normalized_device_slope (&final_slope_dx, &final_slope_dy, stroker->ctm_inverse, NULL))
	_compute_face (d, &spline.final_slope, final_slope_dx, final_slope_dy, stroker, &end);

    if (stroker->has_current_face) {
	status = _cairo_stroker_join (stroker, &stroker->current_face, &start);
	if (status)
	    goto CLEANUP_PEN;
    } else if (!stroker->has_first_face) {
	stroker->first_face = start;
	stroker->has_first_face = TRUE;
    }
    stroker->current_face = end;
    stroker->has_current_face = TRUE;

    extra_points[0] = start.cw;
    extra_points[0].x -= start.point.x;
    extra_points[0].y -= start.point.y;
    extra_points[1] = start.ccw;
    extra_points[1].x -= start.point.x;
    extra_points[1].y -= start.point.y;
    extra_points[2] = end.cw;
    extra_points[2].x -= end.point.x;
    extra_points[2].y -= end.point.y;
    extra_points[3] = end.ccw;
    extra_points[3].x -= end.point.x;
    extra_points[3].y -= end.point.y;

    status = _cairo_pen_add_points (&pen, extra_points, 4);
    if (status)
	goto CLEANUP_PEN;

    status = _cairo_pen_stroke_spline (&pen, &spline, stroker->tolerance, stroker->traps);
    if (status)
	goto CLEANUP_PEN;

  CLEANUP_PEN:
    _cairo_pen_fini (&pen);
  CLEANUP_SPLINE:
    _cairo_spline_fini (&spline);

    stroker->current_point = *d;

    return status;
}



















static cairo_status_t
_cairo_stroker_curve_to_dashed (void *closure,
				cairo_point_t *b,
				cairo_point_t *c,
				cairo_point_t *d)
{
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    cairo_stroker_t *stroker = closure;
    cairo_spline_t spline;
    cairo_point_t *a = &stroker->current_point;
    cairo_line_join_t line_join_save;
    int i;

    status = _cairo_spline_init (&spline, a, b, c, d);
    if (status == CAIRO_INT_STATUS_DEGENERATE)
	return _cairo_stroker_line_to_dashed (closure, d);

    

    if (stroker->pen.num_vertices <= 1)
	goto CLEANUP_SPLINE;

    

    line_join_save = stroker->style->line_join;
    stroker->style->line_join = CAIRO_LINE_JOIN_ROUND;

    status = _cairo_spline_decompose (&spline, stroker->tolerance);
    if (status)
	goto CLEANUP_GSTATE;

    for (i = 1; i < spline.num_points; i++) {
	if (stroker->dashed)
	    status = _cairo_stroker_line_to_dashed (stroker, &spline.points[i]);
	else
	    status = _cairo_stroker_line_to (stroker, &spline.points[i]);
	if (status)
	    break;
    }

  CLEANUP_GSTATE:
    stroker->style->line_join = line_join_save;

  CLEANUP_SPLINE:
    _cairo_spline_fini (&spline);

    return status;
}

static cairo_status_t
_cairo_stroker_close_path (void *closure)
{
    cairo_status_t status;
    cairo_stroker_t *stroker = closure;

    if (stroker->dashed)
	status = _cairo_stroker_line_to_dashed (stroker, &stroker->first_point);
    else
	status = _cairo_stroker_line_to (stroker, &stroker->first_point);
    if (status)
	return status;

    if (stroker->has_first_face && stroker->has_current_face) {
	
	status = _cairo_stroker_join (stroker, &stroker->current_face, &stroker->first_face);
	if (status)
	    return status;
    } else {
	
	status = _cairo_stroker_add_caps (stroker);
	if (status)
	    return status;
    }

    stroker->has_initial_sub_path = FALSE;
    stroker->has_first_face = FALSE;
    stroker->has_current_face = FALSE;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t
_cairo_path_fixed_stroke_rectilinear (cairo_path_fixed_t	*path,
				      cairo_stroke_style_t	*stroke_style,
				      cairo_matrix_t		*ctm,
				      cairo_traps_t		*traps);

cairo_status_t
_cairo_path_fixed_stroke_to_traps (cairo_path_fixed_t	*path,
				   cairo_stroke_style_t	*stroke_style,
				   cairo_matrix_t	*ctm,
				   cairo_matrix_t	*ctm_inverse,
				   double		 tolerance,
				   cairo_traps_t	*traps)
{
    cairo_status_t status;
    cairo_stroker_t stroker;

    




    status = _cairo_path_fixed_stroke_rectilinear (path,
						   stroke_style,
						   ctm,
						   traps);
    if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	return status;

    status = _cairo_stroker_init (&stroker, stroke_style,
			          ctm, ctm_inverse, tolerance,
				  traps);
    if (status)
	return status;

    if (stroker.style->dash)
	status = _cairo_path_fixed_interpret (path,
					      CAIRO_DIRECTION_FORWARD,
					      _cairo_stroker_move_to_dashed,
					      _cairo_stroker_line_to_dashed,
					      _cairo_stroker_curve_to_dashed,
					      _cairo_stroker_close_path,
					      &stroker);
    else
	status = _cairo_path_fixed_interpret (path,
					      CAIRO_DIRECTION_FORWARD,
					      _cairo_stroker_move_to,
					      _cairo_stroker_line_to,
					      _cairo_stroker_curve_to,
					      _cairo_stroker_close_path,
					      &stroker);
    if (status)
	goto BAIL;

    
    status = _cairo_stroker_add_caps (&stroker);

BAIL:
    _cairo_stroker_fini (&stroker);

    return status;
}

typedef struct _cairo_rectilinear_stroker
{
    cairo_stroke_style_t *stroke_style;
    cairo_fixed_t half_line_width;
    cairo_traps_t *traps;
    cairo_point_t current_point;
    cairo_point_t first_point;
    cairo_bool_t open_sub_path;
    int num_segments;
    int segments_size;
    cairo_line_t *segments;
    cairo_line_t segments_embedded[8]; 
} cairo_rectilinear_stroker_t;

static void
_cairo_rectilinear_stroker_init (cairo_rectilinear_stroker_t	*stroker,
				 cairo_stroke_style_t		*stroke_style,
				 cairo_traps_t			*traps)
{
    stroker->stroke_style = stroke_style;
    stroker->half_line_width =
	_cairo_fixed_from_double (stroke_style->line_width / 2.0);
    stroker->traps = traps;
    stroker->open_sub_path = FALSE;
    stroker->segments = stroker->segments_embedded;
    stroker->segments_size = ARRAY_LENGTH (stroker->segments_embedded);
    stroker->num_segments = 0;
}

static void
_cairo_rectilinear_stroker_fini (cairo_rectilinear_stroker_t	*stroker)
{
    if (stroker->segments != stroker->segments_embedded)
	free (stroker->segments);
}

static cairo_status_t
_cairo_rectilinear_stroker_add_segment (cairo_rectilinear_stroker_t	*stroker,
					cairo_point_t			*p1,
					cairo_point_t			*p2)
{

    if (stroker->num_segments == stroker->segments_size) {
	int new_size = stroker->segments_size * 2;
	cairo_line_t *new_segments;

	if (stroker->segments == stroker->segments_embedded) {
	    new_segments = _cairo_malloc_ab (new_size, sizeof (cairo_line_t));
	    if (new_segments == NULL)
		return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	    memcpy (new_segments, stroker->segments,
		    stroker->num_segments * sizeof (cairo_line_t));
	} else {
	    new_segments = _cairo_realloc_ab (stroker->segments,
					      new_size, sizeof (cairo_line_t));
	    if (new_segments == NULL)
		return _cairo_error (CAIRO_STATUS_NO_MEMORY);
	}

	stroker->segments_size = new_size;
	stroker->segments = new_segments;
    }

    stroker->segments[stroker->num_segments].p1 = *p1;
    stroker->segments[stroker->num_segments].p2 = *p2;
    stroker->num_segments++;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_rectilinear_stroker_emit_segments (cairo_rectilinear_stroker_t *stroker)
{
    cairo_status_t status;
    cairo_line_cap_t line_cap = stroker->stroke_style->line_cap;
    cairo_fixed_t half_line_width = stroker->half_line_width;
    int i;

    for (i = 0; i < stroker->num_segments; i++) {
	cairo_point_t *a, *b;
	cairo_bool_t lengthen_initial, shorten_final, lengthen_final;

	a = &stroker->segments[i].p1;
	b = &stroker->segments[i].p2;

	






	




	lengthen_initial = TRUE;
	if (i == 0 && stroker->open_sub_path && line_cap == CAIRO_LINE_CAP_BUTT)
	    lengthen_initial = FALSE;

	







	shorten_final = TRUE;
	lengthen_final = FALSE;
	if (i == stroker->num_segments - 1 && stroker->open_sub_path) {
	    shorten_final = FALSE;
	    if (line_cap == CAIRO_LINE_CAP_SQUARE)
		lengthen_final = TRUE;
	}

	
	if (a->y == b->y) {
	    if (a->x < b->x) {
		if (lengthen_initial)
		    a->x -= half_line_width;
		if (shorten_final)
		    b->x -= half_line_width;
		else if (lengthen_final)
		    b->x += half_line_width;
	    } else {
		if (lengthen_initial)
		    a->x += half_line_width;
		if (shorten_final)
		    b->x += half_line_width;
		else if (lengthen_final)
		    b->x -= half_line_width;
	    }

	    if (a->x > b->x) {
		cairo_point_t *t;

		t = a;
		a = b;
		b = t;
	    }
	} else {
	    if (a->y < b->y) {
		if (lengthen_initial)
		    a->y -= half_line_width;
		if (shorten_final)
		    b->y -= half_line_width;
		else if (lengthen_final)
		    b->y += half_line_width;
	    } else {
		if (lengthen_initial)
		    a->y += half_line_width;
		if (shorten_final)
		    b->y += half_line_width;
		else if (lengthen_final)
		    b->y -= half_line_width;
	    }

	    if (a->y > b->y) {
		cairo_point_t *t;

		t = a;
		a = b;
		b = t;
	    }
	}

	

	if (a->y == b->y) {
	    a->y -= half_line_width;
	    b->y += half_line_width;
	} else {
	    a->x -= half_line_width;
	    b->x += half_line_width;
	}

	status = _cairo_traps_tessellate_rectangle (stroker->traps, a, b);
	if (status)
	    return status;
    }

    stroker->num_segments = 0;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_rectilinear_stroker_move_to (void		*closure,
				    cairo_point_t	*point)
{
    cairo_rectilinear_stroker_t *stroker = closure;
    cairo_status_t status;

    status = _cairo_rectilinear_stroker_emit_segments (stroker);
    if (status)
	return status;

    stroker->current_point = *point;
    stroker->first_point = *point;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_rectilinear_stroker_line_to (void		*closure,
				    cairo_point_t	*point)
{
    cairo_rectilinear_stroker_t *stroker = closure;
    cairo_point_t *a = &stroker->current_point;
    cairo_point_t *b = point;
    cairo_status_t status;

    
    if (! (a->x == b->x || a->y == b->y))
	return CAIRO_INT_STATUS_UNSUPPORTED;

    
    if (a->x == b->x && a->y == b->y)
	return CAIRO_STATUS_SUCCESS;

    status = _cairo_rectilinear_stroker_add_segment (stroker, a, b);

    stroker->current_point = *point;
    stroker->open_sub_path = TRUE;

    return status;
}

static cairo_status_t
_cairo_rectilinear_stroker_close_path (void *closure)
{
    cairo_rectilinear_stroker_t *stroker = closure;
    cairo_status_t status;

    
    if (! stroker->open_sub_path)
	return CAIRO_STATUS_SUCCESS;

    status = _cairo_rectilinear_stroker_line_to (stroker,
						 &stroker->first_point);
    if (status)
	return status;

    stroker->open_sub_path = FALSE;

    status = _cairo_rectilinear_stroker_emit_segments (stroker);
    if (status)
	return status;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t
_cairo_path_fixed_stroke_rectilinear (cairo_path_fixed_t	*path,
				      cairo_stroke_style_t	*stroke_style,
				      cairo_matrix_t		*ctm,
				      cairo_traps_t		*traps)
{
    cairo_rectilinear_stroker_t rectilinear_stroker;
    cairo_int_status_t status;

    









    if (path->has_curve_to)
	return CAIRO_INT_STATUS_UNSUPPORTED;
    if (stroke_style->line_join	!= CAIRO_LINE_JOIN_MITER)
	return CAIRO_INT_STATUS_UNSUPPORTED;
    



    if (stroke_style->miter_limit < M_SQRT2)
	return CAIRO_INT_STATUS_UNSUPPORTED;
    if (stroke_style->dash)
	return CAIRO_INT_STATUS_UNSUPPORTED;
    if (! (stroke_style->line_cap == CAIRO_LINE_CAP_BUTT ||
	   stroke_style->line_cap == CAIRO_LINE_CAP_SQUARE))
    {
	return CAIRO_INT_STATUS_UNSUPPORTED;
    }
    if (! (_cairo_matrix_is_identity (ctm) ||
	   _cairo_matrix_is_translation (ctm)))
    {
	return CAIRO_INT_STATUS_UNSUPPORTED;
    }

    _cairo_rectilinear_stroker_init (&rectilinear_stroker, stroke_style, traps);

    status = _cairo_path_fixed_interpret (path,
					  CAIRO_DIRECTION_FORWARD,
					  _cairo_rectilinear_stroker_move_to,
					  _cairo_rectilinear_stroker_line_to,
					  NULL,
					  _cairo_rectilinear_stroker_close_path,
					  &rectilinear_stroker);
    if (status)
	goto BAIL;

    status = _cairo_rectilinear_stroker_emit_segments (&rectilinear_stroker);

BAIL:
    _cairo_rectilinear_stroker_fini (&rectilinear_stroker);

    if (status)
	_cairo_traps_clear (traps);

    return status;
}
