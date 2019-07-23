



































#include "cairoint.h"
#include "cairo-path-fixed-private.h"

typedef struct cairo_filler {
    double tolerance;
    cairo_traps_t *traps;

    cairo_point_t current_point;

    cairo_polygon_t polygon;
} cairo_filler_t;

static void
_cairo_filler_init (cairo_filler_t *filler, double tolerance, cairo_traps_t *traps);

static void
_cairo_filler_fini (cairo_filler_t *filler);

static cairo_status_t
_cairo_filler_move_to (void *closure, cairo_point_t *point);

static cairo_status_t
_cairo_filler_line_to (void *closure, cairo_point_t *point);

static cairo_status_t
_cairo_filler_curve_to (void *closure,
			cairo_point_t *b,
			cairo_point_t *c,
			cairo_point_t *d);

static cairo_status_t
_cairo_filler_close_path (void *closure);

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
_cairo_filler_move_to (void *closure, cairo_point_t *point)
{
    cairo_filler_t *filler = closure;
    cairo_polygon_t *polygon = &filler->polygon;

    _cairo_polygon_close (polygon);
    _cairo_polygon_move_to (polygon, point);

    filler->current_point = *point;

    return _cairo_polygon_status (&filler->polygon);
}

static cairo_status_t
_cairo_filler_line_to (void *closure, cairo_point_t *point)
{
    cairo_filler_t *filler = closure;
    cairo_polygon_t *polygon = &filler->polygon;

    _cairo_polygon_line_to (polygon, point);

    filler->current_point = *point;

    return _cairo_polygon_status (&filler->polygon);
}

static cairo_status_t
_cairo_filler_curve_to (void *closure,
			cairo_point_t *b,
			cairo_point_t *c,
			cairo_point_t *d)
{
    int i;
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    cairo_filler_t *filler = closure;
    cairo_polygon_t *polygon = &filler->polygon;
    cairo_spline_t spline;

    status = _cairo_spline_init (&spline, &filler->current_point, b, c, d);

    if (status == CAIRO_INT_STATUS_DEGENERATE)
	return CAIRO_STATUS_SUCCESS;

    status = _cairo_spline_decompose (&spline, filler->tolerance);
    if (status)
	goto CLEANUP_SPLINE;

    for (i = 1; i < spline.num_points; i++)
	_cairo_polygon_line_to (polygon, &spline.points[i]);

  CLEANUP_SPLINE:
    _cairo_spline_fini (&spline);

    filler->current_point = *d;

    return status;
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
				  cairo_traps_t		*traps);

cairo_status_t
_cairo_path_fixed_fill_to_traps (cairo_path_fixed_t *path,
				 cairo_fill_rule_t   fill_rule,
				 double              tolerance,
				 cairo_traps_t      *traps)
{
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    cairo_filler_t filler;

    

    status = _cairo_path_fixed_fill_rectangle (path, traps);
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
    if (status)
	goto BAIL;

    _cairo_polygon_close (&filler.polygon);
    status = _cairo_polygon_status (&filler.polygon);
    if (status)
	goto BAIL;

    status = _cairo_bentley_ottmann_tessellate_polygon (filler.traps,
							&filler.polygon,
							fill_rule);
    if (status)
	goto BAIL;

BAIL:
    _cairo_filler_fini (&filler);

    return status;
}








static cairo_int_status_t
_cairo_path_fixed_fill_rectangle (cairo_path_fixed_t	*path,
				  cairo_traps_t		*traps)
{
    if (_cairo_path_fixed_is_box (path, NULL)) {
	cairo_point_t *p = path->buf_head.base.points;
	cairo_point_t *top_left, *bot_right;

	top_left = &p[0];
	bot_right = &p[2];
	if (top_left->x > bot_right->x || top_left->y > bot_right->y) {
	    int n;

	    
	    for (n = 0; n < 4; n++) {
		if (p[n].x <= top_left->x && p[n].y <= top_left->y)
		    top_left = &p[n];
		if (p[n].x >= bot_right->x && p[n].y >= bot_right->y)
		    bot_right = &p[n];
	    }
	}

	return _cairo_traps_tessellate_rectangle (traps, top_left, bot_right);
    }

    return CAIRO_INT_STATUS_UNSUPPORTED;
}
