



































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
    cairo_status_t status;
    cairo_filler_t *filler = closure;
    cairo_polygon_t *polygon = &filler->polygon;

    status = _cairo_polygon_close (polygon);
    if (status)
	return status;

    status = _cairo_polygon_move_to (polygon, point);
    if (status)
	return status;

    filler->current_point = *point;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_filler_line_to (void *closure, cairo_point_t *point)
{
    cairo_status_t status;
    cairo_filler_t *filler = closure;
    cairo_polygon_t *polygon = &filler->polygon;

    status = _cairo_polygon_line_to (polygon, point);
    if (status)
	return status;

    filler->current_point = *point;

    return CAIRO_STATUS_SUCCESS;
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

    _cairo_spline_decompose (&spline, filler->tolerance);
    if (status)
	goto CLEANUP_SPLINE;

    for (i = 1; i < spline.num_points; i++) {
	status = _cairo_polygon_line_to (polygon, &spline.points[i]);
	if (status)
	    break;
    }

  CLEANUP_SPLINE:
    _cairo_spline_fini (&spline);

    filler->current_point = *d;

    return status;
}

static cairo_status_t
_cairo_filler_close_path (void *closure)
{
    cairo_status_t status;
    cairo_filler_t *filler = closure;
    cairo_polygon_t *polygon = &filler->polygon;

    status = _cairo_polygon_close (polygon);
    if (status)
	return status;

    return CAIRO_STATUS_SUCCESS;
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

    status = _cairo_polygon_close (&filler.polygon);
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
    cairo_path_buf_t *buf = path->buf_head;
    int final;

    

    if (buf == NULL || buf->num_ops < 5)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    if (buf->op[0] != CAIRO_PATH_OP_MOVE_TO ||
	buf->op[1] != CAIRO_PATH_OP_LINE_TO ||
	buf->op[2] != CAIRO_PATH_OP_LINE_TO ||
	buf->op[3] != CAIRO_PATH_OP_LINE_TO)
    {
	return CAIRO_INT_STATUS_UNSUPPORTED;
    }

    


    if (buf->op[4] == CAIRO_PATH_OP_LINE_TO) {
	if (buf->points[4].x != buf->points[0].x ||
	    buf->points[4].y != buf->points[0].y)
	{
	    return CAIRO_INT_STATUS_UNSUPPORTED;
	}
    } else if (buf->op[4] != CAIRO_PATH_OP_CLOSE_PATH) {
	return CAIRO_INT_STATUS_UNSUPPORTED;
    }

    


    final = 5;
    if (final < buf->num_ops &&
	buf->op[final] == CAIRO_PATH_OP_CLOSE_PATH)
    {
	final++;
    }
    if (final < buf->num_ops &&
	buf->op[final] == CAIRO_PATH_OP_MOVE_TO)
    {
	final++;
    }
    if (final < buf->num_ops)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    


    if (buf->points[0].y == buf->points[1].y &&
	buf->points[1].x == buf->points[2].x &&
	buf->points[2].y == buf->points[3].y &&
	buf->points[3].x == buf->points[0].x)
    {
	return _cairo_traps_tessellate_convex_quad (traps,
						    buf->points);
    }

    if (buf->points[0].x == buf->points[1].x &&
	buf->points[1].y == buf->points[2].y &&
	buf->points[2].x == buf->points[3].x &&
	buf->points[3].y == buf->points[0].y)
    {
	return _cairo_traps_tessellate_convex_quad (traps,
						    buf->points);
    }

    return CAIRO_INT_STATUS_UNSUPPORTED;
}
