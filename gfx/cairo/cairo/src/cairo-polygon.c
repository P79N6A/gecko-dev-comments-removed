



































#include "cairoint.h"



static cairo_status_t
_cairo_polygon_grow (cairo_polygon_t *polygon);

void
_cairo_polygon_init (cairo_polygon_t *polygon)
{
    polygon->status = CAIRO_STATUS_SUCCESS;

    polygon->num_edges = 0;

    polygon->edges = polygon->edges_embedded;
    polygon->edges_size = ARRAY_LENGTH (polygon->edges_embedded);

    polygon->has_current_point = FALSE;
}

void
_cairo_polygon_fini (cairo_polygon_t *polygon)
{
    if (polygon->edges && polygon->edges != polygon->edges_embedded)
	free (polygon->edges);

    polygon->edges = NULL;
    polygon->edges_size = 0;
    polygon->num_edges = 0;

    polygon->has_current_point = FALSE;
}

cairo_status_t
_cairo_polygon_status (cairo_polygon_t *polygon)
{
    return polygon->status;
}


static cairo_status_t
_cairo_polygon_grow (cairo_polygon_t *polygon)
{
    cairo_edge_t *new_edges;
    int old_size = polygon->edges_size;
    int new_size = 2 * old_size;

    assert (polygon->num_edges <= polygon->edges_size);

    if (polygon->edges == polygon->edges_embedded) {
	new_edges = _cairo_malloc_ab (new_size, sizeof (cairo_edge_t));
	if (new_edges != NULL)
	    memcpy (new_edges, polygon->edges, old_size * sizeof (cairo_edge_t));
    } else {
	new_edges = _cairo_realloc_ab (polygon->edges,
		                       new_size, sizeof (cairo_edge_t));
    }

    if (new_edges == NULL)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    polygon->edges = new_edges;
    polygon->edges_size = new_size;

    return CAIRO_STATUS_SUCCESS;
}

void
_cairo_polygon_add_edge (cairo_polygon_t *polygon, cairo_point_t *p1, cairo_point_t *p2)
{
    cairo_edge_t *edge;

    if (polygon->status)
	return;

    
    if (p1->y == p2->y)
	goto DONE;

    if (polygon->num_edges >= polygon->edges_size) {
	polygon->status = _cairo_polygon_grow (polygon);
	if (polygon->status)
	    return;
    }

    edge = &polygon->edges[polygon->num_edges];
    if (p1->y < p2->y) {
	edge->edge.p1 = *p1;
	edge->edge.p2 = *p2;
	edge->clockWise = 1;
    } else {
	edge->edge.p1 = *p2;
	edge->edge.p2 = *p1;
	edge->clockWise = 0;
    }

    polygon->num_edges++;

  DONE:
    _cairo_polygon_move_to (polygon, p2);
}

void
_cairo_polygon_move_to (cairo_polygon_t *polygon, cairo_point_t *point)
{
    if (polygon->status)
	return;

    if (! polygon->has_current_point)
	polygon->first_point = *point;

    polygon->current_point = *point;
    polygon->has_current_point = TRUE;
}

void
_cairo_polygon_line_to (cairo_polygon_t *polygon, cairo_point_t *point)
{
    if (polygon->status)
	return;

    if (polygon->has_current_point) {
	_cairo_polygon_add_edge (polygon, &polygon->current_point, point);
    } else {
	_cairo_polygon_move_to (polygon, point);
    }
}

void
_cairo_polygon_close (cairo_polygon_t *polygon)
{
    if (polygon->status)
	return;

    if (polygon->has_current_point) {
	_cairo_polygon_add_edge (polygon,
				 &polygon->current_point,
				 &polygon->first_point);

	polygon->has_current_point = FALSE;
    }
}
