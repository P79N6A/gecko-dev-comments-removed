





































#include "cairoint.h"

static int
_cairo_pen_vertices_needed (double tolerance,
			    double radius,
			    const cairo_matrix_t *matrix);

static void
_cairo_pen_compute_slopes (cairo_pen_t *pen);

cairo_status_t
_cairo_pen_init (cairo_pen_t	*pen,
		 double		 radius,
		 double		 tolerance,
		 const cairo_matrix_t	*ctm)
{
    int i;
    int reflect;

    if (CAIRO_INJECT_FAULT ())
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    VG (VALGRIND_MAKE_MEM_UNDEFINED (pen, sizeof (cairo_pen_t)));

    pen->radius = radius;
    pen->tolerance = tolerance;

    reflect = _cairo_matrix_compute_determinant (ctm) < 0.;

    pen->num_vertices = _cairo_pen_vertices_needed (tolerance,
						    radius,
						    ctm);

    if (pen->num_vertices > ARRAY_LENGTH (pen->vertices_embedded)) {
	pen->vertices = _cairo_malloc_ab (pen->num_vertices,
					  sizeof (cairo_pen_vertex_t));
	if (unlikely (pen->vertices == NULL))
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    } else {
	pen->vertices = pen->vertices_embedded;
    }

    





    for (i=0; i < pen->num_vertices; i++) {
	double theta = 2 * M_PI * i / (double) pen->num_vertices;
	double dx = radius * cos (reflect ? -theta : theta);
	double dy = radius * sin (reflect ? -theta : theta);
	cairo_pen_vertex_t *v = &pen->vertices[i];
	cairo_matrix_transform_distance (ctm, &dx, &dy);
	v->point.x = _cairo_fixed_from_double (dx);
	v->point.y = _cairo_fixed_from_double (dy);
    }

    _cairo_pen_compute_slopes (pen);

    return CAIRO_STATUS_SUCCESS;
}

void
_cairo_pen_fini (cairo_pen_t *pen)
{
    if (pen->vertices != pen->vertices_embedded)
	free (pen->vertices);


    VG (VALGRIND_MAKE_MEM_NOACCESS (pen, sizeof (cairo_pen_t)));
}

cairo_status_t
_cairo_pen_init_copy (cairo_pen_t *pen, const cairo_pen_t *other)
{
    VG (VALGRIND_MAKE_MEM_UNDEFINED (pen, sizeof (cairo_pen_t)));

    *pen = *other;

    if (CAIRO_INJECT_FAULT ())
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    pen->vertices = pen->vertices_embedded;
    if (pen->num_vertices) {
	if (pen->num_vertices > ARRAY_LENGTH (pen->vertices_embedded)) {
	    pen->vertices = _cairo_malloc_ab (pen->num_vertices,
					      sizeof (cairo_pen_vertex_t));
	    if (unlikely (pen->vertices == NULL))
		return _cairo_error (CAIRO_STATUS_NO_MEMORY);
	}

	memcpy (pen->vertices, other->vertices,
		pen->num_vertices * sizeof (cairo_pen_vertex_t));
    }

    return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_pen_add_points (cairo_pen_t *pen, cairo_point_t *point, int num_points)
{
    cairo_status_t status;
    int num_vertices;
    int i;

    if (CAIRO_INJECT_FAULT ())
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    num_vertices = pen->num_vertices + num_points;
    if (num_vertices > ARRAY_LENGTH (pen->vertices_embedded) ||
	pen->vertices != pen->vertices_embedded)
    {
	cairo_pen_vertex_t *vertices;

	if (pen->vertices == pen->vertices_embedded) {
	    vertices = _cairo_malloc_ab (num_vertices,
		                         sizeof (cairo_pen_vertex_t));
	    if (unlikely (vertices == NULL))
		return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	    memcpy (vertices, pen->vertices,
		    pen->num_vertices * sizeof (cairo_pen_vertex_t));
	} else {
	    vertices = _cairo_realloc_ab (pen->vertices,
					  num_vertices,
					  sizeof (cairo_pen_vertex_t));
	    if (unlikely (vertices == NULL))
		return _cairo_error (CAIRO_STATUS_NO_MEMORY);
	}

	pen->vertices = vertices;
    }

    pen->num_vertices = num_vertices;

    
    for (i=0; i < num_points; i++)
	pen->vertices[pen->num_vertices-num_points+i].point = point[i];

    status = _cairo_hull_compute (pen->vertices, &pen->num_vertices);
    if (unlikely (status))
	return status;

    _cairo_pen_compute_slopes (pen);

    return CAIRO_STATUS_SUCCESS;
}






















































































static int
_cairo_pen_vertices_needed (double	    tolerance,
			    double	    radius,
			    const cairo_matrix_t  *matrix)
{
    





    double  major_axis = _cairo_matrix_transformed_circle_major_axis (matrix,
								      radius);

    


    int	    num_vertices;

    
    if (tolerance >= major_axis) {
	num_vertices = 4;
    } else {
	double delta = acos (1 - tolerance / major_axis);
	num_vertices = ceil (M_PI / delta);

	
	if (num_vertices % 2)
	    num_vertices++;

	
	if (num_vertices < 4)
	    num_vertices = 4;
    }

    return num_vertices;
}

static void
_cairo_pen_compute_slopes (cairo_pen_t *pen)
{
    int i, i_prev;
    cairo_pen_vertex_t *prev, *v, *next;

    for (i=0, i_prev = pen->num_vertices - 1;
	 i < pen->num_vertices;
	 i_prev = i++) {
	prev = &pen->vertices[i_prev];
	v = &pen->vertices[i];
	next = &pen->vertices[(i + 1) % pen->num_vertices];

	_cairo_slope_init (&v->slope_cw, &prev->point, &v->point);
	_cairo_slope_init (&v->slope_ccw, &v->point, &next->point);
    }
}













int
_cairo_pen_find_active_cw_vertex_index (const cairo_pen_t *pen,
					const cairo_slope_t *slope)
{
    int i;

    for (i=0; i < pen->num_vertices; i++) {
	if ((_cairo_slope_compare (slope, &pen->vertices[i].slope_ccw) < 0) &&
	    (_cairo_slope_compare (slope, &pen->vertices[i].slope_cw) >= 0))
	    break;
    }

    




    if (i == pen->num_vertices)
	i = 0;

    return i;
}






int
_cairo_pen_find_active_ccw_vertex_index (const cairo_pen_t *pen,
					 const cairo_slope_t *slope)
{
    cairo_slope_t slope_reverse;
    int i;

    slope_reverse = *slope;
    slope_reverse.dx = -slope_reverse.dx;
    slope_reverse.dy = -slope_reverse.dy;

    for (i=pen->num_vertices-1; i >= 0; i--) {
	if ((_cairo_slope_compare (&pen->vertices[i].slope_ccw, &slope_reverse) >= 0) &&
	    (_cairo_slope_compare (&pen->vertices[i].slope_cw, &slope_reverse) < 0))
	    break;
    }

    




    if (i < 0)
	i = pen->num_vertices - 1;

    return i;
}

static int
_cairo_pen_stroke_spline_add_convolved_point (cairo_pen_stroke_spline_t	*stroker,
					      const cairo_point_t *last_point,
					      const cairo_slope_t *slope,
					      cairo_point_t *last_hull_point,
					      int active,
					      int step)
{
    do {
	cairo_point_t hull_point;

	hull_point.x = last_point->x + stroker->pen.vertices[active].point.x;
	hull_point.y = last_point->y + stroker->pen.vertices[active].point.y;
	_cairo_polygon_add_edge (&stroker->polygon,
				 last_hull_point, &hull_point,
				 step);
	*last_hull_point = hull_point;

	









	if (_cairo_slope_compare (slope,
				  &stroker->pen.vertices[active].slope_ccw) > 0)
	{
	    if (++active == stroker->pen.num_vertices)
		active = 0;
	}
	else if (_cairo_slope_compare (slope,
				       &stroker->pen.vertices[active].slope_cw) < 0)
	{
	    if (--active == -1)
		active = stroker->pen.num_vertices - 1;
	}
	else
	{
	    return active;
	}
    } while (TRUE);
}





cairo_status_t
_cairo_pen_stroke_spline (cairo_pen_stroke_spline_t	*stroker,
			  double			 tolerance,
			  cairo_traps_t			*traps)
{
    cairo_status_t status;
    cairo_slope_t slope;

    

    if (stroker->pen.num_vertices <= 1)
	return CAIRO_STATUS_SUCCESS;

    
    slope = stroker->spline.initial_slope;
    stroker->forward_vertex =
	_cairo_pen_find_active_cw_vertex_index (&stroker->pen, &slope);
    stroker->forward_hull_point.x = stroker->last_point.x +
	stroker->pen.vertices[stroker->forward_vertex].point.x;
    stroker->forward_hull_point.y = stroker->last_point.y +
	stroker->pen.vertices[stroker->forward_vertex].point.y;

    slope.dx = -slope.dx;
    slope.dy = -slope.dy;
    stroker->backward_vertex =
	_cairo_pen_find_active_cw_vertex_index (&stroker->pen, &slope);
    stroker->backward_hull_point.x = stroker->last_point.x +
	stroker->pen.vertices[stroker->backward_vertex].point.x;
    stroker->backward_hull_point.y = stroker->last_point.y +
	stroker->pen.vertices[stroker->backward_vertex].point.y;

    _cairo_polygon_add_edge (&stroker->polygon,
			     &stroker->backward_hull_point,
			     &stroker->forward_hull_point,
			     1);

    status = _cairo_spline_decompose (&stroker->spline, tolerance);
    if (unlikely (status))
	return status;

    
    slope = stroker->spline.final_slope;
    _cairo_pen_stroke_spline_add_convolved_point (stroker,
						  &stroker->last_point,
						  &slope,
						  &stroker->forward_hull_point,
						  stroker->forward_vertex,
						  1);

    slope.dx = -slope.dx;
    slope.dy = -slope.dy;
    _cairo_pen_stroke_spline_add_convolved_point (stroker,
						  &stroker->last_point,
						  &slope,
						  &stroker->backward_hull_point,
						  stroker->backward_vertex,
						  -1);

    _cairo_polygon_add_edge (&stroker->polygon,
			     &stroker->forward_hull_point,
			     &stroker->backward_hull_point,
			     1);

    status = _cairo_polygon_status (&stroker->polygon);
    if (unlikely (status))
	return status;

    status = _cairo_bentley_ottmann_tessellate_polygon (traps,
							&stroker->polygon,
							CAIRO_FILL_RULE_WINDING);

    return status;
}

static cairo_status_t
_cairo_pen_stroke_spline_add_point (void		    *closure,
				    const cairo_point_t	    *point)
{
    cairo_pen_stroke_spline_t	*stroker = closure;
    cairo_slope_t slope;

    _cairo_slope_init (&slope, &stroker->last_point, point);
    stroker->forward_vertex =
	_cairo_pen_stroke_spline_add_convolved_point (stroker,
						      &stroker->last_point,
						      &slope,
						      &stroker->forward_hull_point,
						      stroker->forward_vertex,
						      1);

    slope.dx = -slope.dx;
    slope.dy = -slope.dy;
    stroker->backward_vertex =
	_cairo_pen_stroke_spline_add_convolved_point (stroker,
						      &stroker->last_point,
						      &slope,
						      &stroker->backward_hull_point,
						      stroker->backward_vertex,
						      -1);
    stroker->last_point = *point;

    return CAIRO_STATUS_SUCCESS;
}

cairo_int_status_t
_cairo_pen_stroke_spline_init (cairo_pen_stroke_spline_t *stroker,
			       const cairo_pen_t *pen,
			       const cairo_point_t *a,
			       const cairo_point_t *b,
			       const cairo_point_t *c,
			       const cairo_point_t *d)
{
    cairo_int_status_t status;

    if (! _cairo_spline_init (&stroker->spline,
			      _cairo_pen_stroke_spline_add_point,
			      stroker,
			      a, b, c, d))
    {
	return CAIRO_INT_STATUS_DEGENERATE;
    }

    status = _cairo_pen_init_copy (&stroker->pen, pen);
    if (unlikely (status))
	return status;

    _cairo_polygon_init (&stroker->polygon);

    stroker->last_point = *a;

    return CAIRO_STATUS_SUCCESS;
}

void
_cairo_pen_stroke_spline_fini (cairo_pen_stroke_spline_t *stroker)
{
    _cairo_polygon_fini (&stroker->polygon);
    _cairo_pen_fini (&stroker->pen);
}
