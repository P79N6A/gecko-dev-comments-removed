



































#include "cairoint.h"

static cairo_status_t
_cairo_spline_grow (cairo_spline_t *spline);

static cairo_status_t
_cairo_spline_add_point (cairo_spline_t *spline, const cairo_point_t *point);

static void
_lerp_half (const cairo_point_t *a, const cairo_point_t *b, cairo_point_t *result);

static void
_de_casteljau (cairo_spline_knots_t *s1, cairo_spline_knots_t *s2);

static double
_cairo_spline_error_squared (const cairo_spline_knots_t *spline);

static cairo_status_t
_cairo_spline_decompose_into (cairo_spline_knots_t *spline, double tolerance_squared, cairo_spline_t *result);

cairo_int_status_t
_cairo_spline_init (cairo_spline_t *spline,
		    const cairo_point_t *a, const cairo_point_t *b,
		    const cairo_point_t *c, const cairo_point_t *d)
{
    spline->knots.a = *a;
    spline->knots.b = *b;
    spline->knots.c = *c;
    spline->knots.d = *d;

    if (a->x != b->x || a->y != b->y)
	_cairo_slope_init (&spline->initial_slope, &spline->knots.a, &spline->knots.b);
    else if (a->x != c->x || a->y != c->y)
	_cairo_slope_init (&spline->initial_slope, &spline->knots.a, &spline->knots.c);
    else if (a->x != d->x || a->y != d->y)
	_cairo_slope_init (&spline->initial_slope, &spline->knots.a, &spline->knots.d);
    else
	return CAIRO_INT_STATUS_DEGENERATE;

    if (c->x != d->x || c->y != d->y)
	_cairo_slope_init (&spline->final_slope, &spline->knots.c, &spline->knots.d);
    else if (b->x != d->x || b->y != d->y)
	_cairo_slope_init (&spline->final_slope, &spline->knots.b, &spline->knots.d);
    else
	_cairo_slope_init (&spline->final_slope, &spline->knots.a, &spline->knots.d);

    spline->points = spline->points_embedded;
    spline->points_size = ARRAY_LENGTH (spline->points_embedded);
    spline->num_points = 0;

    return CAIRO_STATUS_SUCCESS;
}

void
_cairo_spline_fini (cairo_spline_t *spline)
{
    if (spline->points != spline->points_embedded)
	free (spline->points);

    spline->points = spline->points_embedded;
    spline->points_size = ARRAY_LENGTH (spline->points_embedded);
    spline->num_points = 0;
}


static cairo_status_t
_cairo_spline_grow (cairo_spline_t *spline)
{
    cairo_point_t *new_points;
    int old_size = spline->points_size;
    int new_size = 2 * MAX (old_size, 16);

    assert (spline->num_points <= spline->points_size);

    if (spline->points == spline->points_embedded) {
	new_points = _cairo_malloc_ab (new_size, sizeof (cairo_point_t));
	if (new_points)
	    memcpy (new_points, spline->points, old_size * sizeof (cairo_point_t));
    } else {
	new_points = _cairo_realloc_ab (spline->points,
		                        new_size, sizeof (cairo_point_t));
    }

    if (new_points == NULL)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    spline->points = new_points;
    spline->points_size = new_size;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_spline_add_point (cairo_spline_t *spline, const cairo_point_t *point)
{
    cairo_status_t status;
    cairo_point_t *prev;

    if (spline->num_points) {
	prev = &spline->points[spline->num_points - 1];
	if (prev->x == point->x && prev->y == point->y)
	    return CAIRO_STATUS_SUCCESS;
    }

    if (spline->num_points >= spline->points_size) {
	status = _cairo_spline_grow (spline);
	if (status)
	    return status;
    }

    spline->points[spline->num_points] = *point;
    spline->num_points++;

    return CAIRO_STATUS_SUCCESS;
}

static void
_lerp_half (const cairo_point_t *a, const cairo_point_t *b, cairo_point_t *result)
{
    result->x = a->x + ((b->x - a->x) >> 1);
    result->y = a->y + ((b->y - a->y) >> 1);
}

static void
_de_casteljau (cairo_spline_knots_t *s1, cairo_spline_knots_t *s2)
{
    cairo_point_t ab, bc, cd;
    cairo_point_t abbc, bccd;
    cairo_point_t final;

    _lerp_half (&s1->a, &s1->b, &ab);
    _lerp_half (&s1->b, &s1->c, &bc);
    _lerp_half (&s1->c, &s1->d, &cd);
    _lerp_half (&ab, &bc, &abbc);
    _lerp_half (&bc, &cd, &bccd);
    _lerp_half (&abbc, &bccd, &final);

    s2->a = final;
    s2->b = bccd;
    s2->c = cd;
    s2->d = s1->d;

    s1->b = ab;
    s1->c = abbc;
    s1->d = final;
}



static double
_cairo_spline_error_squared (const cairo_spline_knots_t *knots)
{
    double bdx, bdy, berr;
    double cdx, cdy, cerr;

    





    bdx = _cairo_fixed_to_double (knots->b.x - knots->a.x);
    bdy = _cairo_fixed_to_double (knots->b.y - knots->a.y);

    cdx = _cairo_fixed_to_double (knots->c.x - knots->a.x);
    cdy = _cairo_fixed_to_double (knots->c.y - knots->a.y);

    if (knots->a.x != knots->d.x || knots->a.y != knots->d.y) {
	double dx, dy, u, v;

	dx = _cairo_fixed_to_double (knots->d.x - knots->a.x);
	dy = _cairo_fixed_to_double (knots->d.y - knots->a.y);
	 v = dx * dx + dy * dy;

	u = bdx * dx + bdy * dy;
	if (u <= 0) {
	    


	} else if (u >= v) {
	    bdx -= dx;
	    bdy -= dy;
	} else {
	    bdx -= u/v * dx;
	    bdy -= u/v * dy;
	}

	u = cdx * dx + cdy * dy;
	if (u <= 0) {
	    


	} else if (u >= v) {
	    cdx -= dx;
	    cdy -= dy;
	} else {
	    cdx -= u/v * dx;
	    cdy -= u/v * dy;
	}
    }

    berr = bdx * bdx + bdy * bdy;
    cerr = cdx * cdx + cdy * cdy;
    if (berr > cerr)
	return berr;
    else
	return cerr;
}

static cairo_status_t
_cairo_spline_decompose_into (cairo_spline_knots_t *s1, double tolerance_squared, cairo_spline_t *result)
{
    cairo_spline_knots_t s2;
    cairo_status_t status;

    if (_cairo_spline_error_squared (s1) < tolerance_squared)
	return _cairo_spline_add_point (result, &s1->a);

    _de_casteljau (s1, &s2);

    status = _cairo_spline_decompose_into (s1, tolerance_squared, result);
    if (status)
	return status;

    status = _cairo_spline_decompose_into (&s2, tolerance_squared, result);
    if (status)
	return status;

    return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_spline_decompose (cairo_spline_t *spline, double tolerance)
{
    cairo_status_t status;
    cairo_spline_knots_t s1;

    
    spline->num_points = 0;

    s1 = spline->knots;
    status = _cairo_spline_decompose_into (&s1, tolerance * tolerance, spline);
    if (status)
	return status;

    status = _cairo_spline_add_point (spline, &spline->knots.d);
    if (status)
	return status;

    return CAIRO_STATUS_SUCCESS;
}
