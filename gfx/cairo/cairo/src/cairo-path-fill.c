



































#include "cairoint.h"
#include "cairo-path-fixed-private.h"

typedef struct cairo_filler {
    double tolerance;
    cairo_traps_t *traps;

    cairo_point_t current_point;

    cairo_polygon_t polygon;
} cairo_filler_t;

static void
_cairo_filler_init (cairo_filler_t *filler, double tolerance, cairo_traps_t *traps)
{
    filler->tolerance = tolerance;
    filler->traps = traps;

    filler->current_point.x = 0;
    filler->current_point.y = 0;

    _cairo_polygon_init (&filler->polygon);
}

static void
_cairo_filler_fini (cairo_filler_t *filler)
{
    _cairo_polygon_fini (&filler->polygon);
}

static cairo_status_t
_cairo_filler_move_to (void *closure,
		       const cairo_point_t *point)
{
    cairo_filler_t *filler = closure;
    cairo_polygon_t *polygon = &filler->polygon;

    _cairo_polygon_close (polygon);
    _cairo_polygon_move_to (polygon, point);

    filler->current_point = *point;

    return _cairo_polygon_status (&filler->polygon);
}

static cairo_status_t
_cairo_filler_line_to (void *closure,
		       const cairo_point_t *point)
{
    cairo_filler_t *filler = closure;
    cairo_polygon_t *polygon = &filler->polygon;

    _cairo_polygon_line_to (polygon, point);

    filler->current_point = *point;

    return _cairo_polygon_status (&filler->polygon);
}

static cairo_status_t
_cairo_filler_curve_to (void *closure,
			const cairo_point_t *b,
			const cairo_point_t *c,
			const cairo_point_t *d)
{
    cairo_filler_t *filler = closure;
    cairo_spline_t spline;

    if (! _cairo_spline_init (&spline,
			      _cairo_filler_line_to,
			      filler,
			      &filler->current_point, b, c, d))
    {
	return CAIRO_STATUS_SUCCESS;
    }

    return _cairo_spline_decompose (&spline, filler->tolerance);
}

static cairo_status_t
_cairo_filler_close_path (void *closure)
{
    cairo_filler_t *filler = closure;
    cairo_polygon_t *polygon = &filler->polygon;

    _cairo_polygon_close (polygon);

    return _cairo_polygon_status (polygon);
}

static cairo_int_status_t
_cairo_path_fixed_fill_rectangle (cairo_path_fixed_t	*path,
				  cairo_fill_rule_t	 fill_rule,
				  cairo_traps_t		*traps);

cairo_status_t
_cairo_path_fixed_fill_to_traps (cairo_path_fixed_t *path,
				 cairo_fill_rule_t   fill_rule,
				 double              tolerance,
				 cairo_traps_t      *traps)
{
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    cairo_filler_t filler;

    

    status = _cairo_path_fixed_fill_rectangle (path, fill_rule, traps);
    if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	return status;

    _cairo_filler_init (&filler, tolerance, traps);

    status = _cairo_path_fixed_interpret (path,
					  CAIRO_DIRECTION_FORWARD,
					  _cairo_filler_move_to,
					  _cairo_filler_line_to,
					  _cairo_filler_curve_to,
					  _cairo_filler_close_path,
					  &filler);
    if (unlikely (status))
	goto BAIL;

    _cairo_polygon_close (&filler.polygon);
    status = _cairo_polygon_status (&filler.polygon);
    if (unlikely (status))
	goto BAIL;

    status = _cairo_bentley_ottmann_tessellate_polygon (filler.traps,
							&filler.polygon,
							fill_rule);
    if (unlikely (status))
	goto BAIL;

BAIL:
    _cairo_filler_fini (&filler);

    return status;
}








static cairo_int_status_t
_cairo_path_fixed_fill_rectangle (cairo_path_fixed_t	*path,
				  cairo_fill_rule_t	 fill_rule,
				  cairo_traps_t		*traps)
{
    cairo_box_t box;

    if (_cairo_path_fixed_is_box (path, &box)) {
	if (box.p1.x > box.p2.x) {
	    cairo_fixed_t t;

	    t = box.p1.x;
	    box.p1.x = box.p2.x;
	    box.p2.x = t;
	}

	if (box.p1.y > box.p2.y) {
	    cairo_fixed_t t;

	    t = box.p1.y;
	    box.p1.y = box.p2.y;
	    box.p2.y = t;
	}

	return _cairo_traps_tessellate_rectangle (traps, &box.p1, &box.p2);
    } else if (fill_rule == CAIRO_FILL_RULE_WINDING) {
	cairo_path_fixed_iter_t iter;
	int last_cw = -1;

	


	_cairo_path_fixed_iter_init (&iter, path);
	while (_cairo_path_fixed_iter_is_fill_box (&iter, &box)) {
	    cairo_status_t status;
	    int cw = 0;

	    if (box.p1.x > box.p2.x) {
		cairo_fixed_t t;

		t = box.p1.x;
		box.p1.x = box.p2.x;
		box.p2.x = t;

		cw = ! cw;
	    }

	    if (box.p1.y > box.p2.y) {
		cairo_fixed_t t;

		t = box.p1.y;
		box.p1.y = box.p2.y;
		box.p2.y = t;

		cw = ! cw;
	    }

	    if (last_cw < 0) {
		last_cw = cw;
	    } else if (last_cw != cw) {
		_cairo_traps_clear (traps);
		return CAIRO_INT_STATUS_UNSUPPORTED;
	    }

	    status = _cairo_traps_tessellate_rectangle (traps,
							&box.p1, &box.p2);
	    if (unlikely (status))
		return status;
	}
	if (_cairo_path_fixed_iter_at_end (&iter))
	    return CAIRO_STATUS_SUCCESS;

	_cairo_traps_clear (traps);
    }

    return CAIRO_INT_STATUS_UNSUPPORTED;
}
