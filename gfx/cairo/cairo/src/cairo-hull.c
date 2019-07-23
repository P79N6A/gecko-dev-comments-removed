



































#include "cairoint.h"

typedef struct cairo_hull
{
    cairo_point_t point;
    cairo_slope_t slope;
    int discard;
    int id;
} cairo_hull_t;

static cairo_hull_t *
_cairo_hull_create (cairo_pen_vertex_t *vertices, int num_vertices)
{
    int i;
    cairo_hull_t *hull;
    cairo_point_t *p, *extremum, tmp;

    extremum = &vertices[0].point;
    for (i = 1; i < num_vertices; i++) {
	p = &vertices[i].point;
	if (p->y < extremum->y || (p->y == extremum->y && p->x < extremum->x))
	    extremum = p;
    }
    
    tmp = *extremum;
    *extremum = vertices[0].point;
    vertices[0].point = tmp;

    hull = malloc (num_vertices * sizeof (cairo_hull_t));
    if (hull == NULL)
	return NULL;

    for (i = 0; i < num_vertices; i++) {
	hull[i].point = vertices[i].point;
	_cairo_slope_init (&hull[i].slope, &hull[0].point, &hull[i].point);

        
        hull[i].id = i;

        
        hull[i].discard = 0;

	
	if (i != 0 && hull[i].slope.dx == 0 && hull[i].slope.dy == 0)
	    hull[i].discard = 1;
    }

    return hull;
}

static int
_cairo_hull_vertex_compare (const void *av, const void *bv)
{
    cairo_hull_t *a = (cairo_hull_t *) av;
    cairo_hull_t *b = (cairo_hull_t *) bv;
    int ret;

    ret = _cairo_slope_compare (&a->slope, &b->slope);

    


    if (ret == 0) {
	cairo_fixed_48_16_t a_dist, b_dist;
	a_dist = ((cairo_fixed_48_16_t) a->slope.dx * a->slope.dx +
		  (cairo_fixed_48_16_t) a->slope.dy * a->slope.dy);
	b_dist = ((cairo_fixed_48_16_t) b->slope.dx * b->slope.dx +
		  (cairo_fixed_48_16_t) b->slope.dy * b->slope.dy);
	




	if (a_dist < b_dist || (a_dist == b_dist && a->id < b->id)) {
	    a->discard = 1;
	    ret = -1;
	} else {
	    b->discard = 1;
	    ret = 1;
	}
    }

    return ret;
}

static int
_cairo_hull_prev_valid (cairo_hull_t *hull, int num_hull, int index)
{
    do {
	
	index--;
    } while (hull[index].discard);

    return index;
}

static int
_cairo_hull_next_valid (cairo_hull_t *hull, int num_hull, int index)
{
    do {
	index = (index + 1) % num_hull;
    } while (hull[index].discard);

    return index;
}

static cairo_status_t
_cairo_hull_eliminate_concave (cairo_hull_t *hull, int num_hull)
{
    int i, j, k;
    cairo_slope_t slope_ij, slope_jk;

    i = 0;
    j = _cairo_hull_next_valid (hull, num_hull, i);
    k = _cairo_hull_next_valid (hull, num_hull, j);

    do {
	_cairo_slope_init (&slope_ij, &hull[i].point, &hull[j].point);
	_cairo_slope_init (&slope_jk, &hull[j].point, &hull[k].point);

	
	if (_cairo_slope_compare (&slope_ij, &slope_jk) >= 0) {
	    if (i == k)
		return CAIRO_STATUS_SUCCESS;
	    hull[j].discard = 1;
	    j = i;
	    i = _cairo_hull_prev_valid (hull, num_hull, j);
	} else {
	    i = j;
	    j = k;
	    k = _cairo_hull_next_valid (hull, num_hull, j);
	}
    } while (j != 0);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_hull_to_pen (cairo_hull_t *hull, cairo_pen_vertex_t *vertices, int *num_vertices)
{
    int i, j = 0;

    for (i = 0; i < *num_vertices; i++) {
	if (hull[i].discard)
	    continue;
	vertices[j++].point = hull[i].point;
    }

    *num_vertices = j;

    return CAIRO_STATUS_SUCCESS;
}



cairo_status_t
_cairo_hull_compute (cairo_pen_vertex_t *vertices, int *num_vertices)
{
    cairo_hull_t *hull;
    int num_hull = *num_vertices;

    hull = _cairo_hull_create (vertices, num_hull);
    if (hull == NULL)
	return CAIRO_STATUS_NO_MEMORY;

    qsort (hull + 1, num_hull - 1,
	   sizeof (cairo_hull_t), _cairo_hull_vertex_compare);

    _cairo_hull_eliminate_concave (hull, num_hull);

    _cairo_hull_to_pen (hull, vertices, num_vertices);

    free (hull);

    return CAIRO_STATUS_SUCCESS;
}
