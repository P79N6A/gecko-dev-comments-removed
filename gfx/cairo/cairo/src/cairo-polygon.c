



































#include "cairoint.h"

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
    if (polygon->edges != polygon->edges_embedded)
	free (polygon->edges);
}


static cairo_bool_t
_cairo_polygon_grow (cairo_polygon_t *polygon)
{
    cairo_edge_t *new_edges;
    int old_size = polygon->edges_size;
    int new_size = 4 * old_size;

    if (CAIRO_INJECT_FAULT ()) {
	polygon->status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	return FALSE;
    }

    if (polygon->edges == polygon->edges_embedded) {
	new_edges = _cairo_malloc_ab (new_size, sizeof (cairo_edge_t));
	if (new_edges != NULL)
	    memcpy (new_edges, polygon->edges, old_size * sizeof (cairo_edge_t));
    } else {
	new_edges = _cairo_realloc_ab (polygon->edges,
		                       new_size, sizeof (cairo_edge_t));
    }

    if (unlikely (new_edges == NULL)) {
	polygon->status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	return FALSE;
    }

    polygon->edges = new_edges;
    polygon->edges_size = new_size;

    return TRUE;
}

void
_cairo_polygon_add_edge (cairo_polygon_t *polygon,
			 const cairo_point_t *p1,
			 const cairo_point_t *p2,
			 int		      dir)
{
    cairo_edge_t *edge;

    
    if (p1->y == p2->y)
	return;

    if (polygon->num_edges == polygon->edges_size) {
	if (! _cairo_polygon_grow (polygon))
	    return;
    }

    edge = &polygon->edges[polygon->num_edges++];
    if (p1->y < p2->y) {
	edge->edge.p1 = *p1;
	edge->edge.p2 = *p2;
	edge->dir = dir;
    } else {
	edge->edge.p1 = *p2;
	edge->edge.p2 = *p1;
	edge->dir = -dir;
    }
}

void
_cairo_polygon_move_to (cairo_polygon_t *polygon,
			const cairo_point_t *point)
{
    if (! polygon->has_current_point)
	polygon->first_point = *point;

    polygon->current_point = *point;
    polygon->has_current_point = TRUE;
}

void
_cairo_polygon_line_to (cairo_polygon_t *polygon,
			const cairo_point_t *point)
{
    if (polygon->has_current_point)
	_cairo_polygon_add_edge (polygon, &polygon->current_point, point, 1);

    _cairo_polygon_move_to (polygon, point);
}

void
_cairo_polygon_close (cairo_polygon_t *polygon)
{
    if (polygon->has_current_point) {
	_cairo_polygon_add_edge (polygon,
				 &polygon->current_point,
				 &polygon->first_point,
				 1);

	polygon->has_current_point = FALSE;
    }
}
