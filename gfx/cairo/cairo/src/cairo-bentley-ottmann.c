





































#include "cairoint.h"

#include "cairo-freelist-private.h"
#include "cairo-combsort-private.h"

#define DEBUG_PRINT_STATE 0
#define DEBUG_EVENTS 0
#define DEBUG_TRAPS 0

typedef cairo_point_t cairo_bo_point32_t;

typedef struct _cairo_bo_intersect_ordinate {
    int32_t ordinate;
    enum { EXACT, INEXACT } exactness;
} cairo_bo_intersect_ordinate_t;

typedef struct _cairo_bo_intersect_point {
    cairo_bo_intersect_ordinate_t x;
    cairo_bo_intersect_ordinate_t y;
} cairo_bo_intersect_point_t;

typedef struct _cairo_bo_edge cairo_bo_edge_t;
typedef struct _cairo_bo_trap cairo_bo_trap_t;


struct _cairo_bo_trap {
    cairo_bo_edge_t *right;
    int32_t top;
};

struct _cairo_bo_edge {
    cairo_edge_t edge;
    cairo_bo_edge_t *prev;
    cairo_bo_edge_t *next;
    cairo_bo_trap_t deferred_trap;
};


#define PQ_PARENT_INDEX(i) ((i) >> 1)
#define PQ_FIRST_ENTRY 1


#define PQ_LEFT_CHILD_INDEX(i) ((i) << 1)

typedef enum {
    CAIRO_BO_EVENT_TYPE_STOP,
    CAIRO_BO_EVENT_TYPE_INTERSECTION,
    CAIRO_BO_EVENT_TYPE_START
} cairo_bo_event_type_t;

typedef struct _cairo_bo_event {
    cairo_bo_event_type_t type;
    cairo_point_t point;
} cairo_bo_event_t;

typedef struct _cairo_bo_start_event {
    cairo_bo_event_type_t type;
    cairo_point_t point;
    cairo_bo_edge_t edge;
} cairo_bo_start_event_t;

typedef struct _cairo_bo_queue_event {
    cairo_bo_event_type_t type;
    cairo_point_t point;
    cairo_bo_edge_t *e1;
    cairo_bo_edge_t *e2;
} cairo_bo_queue_event_t;

typedef struct _pqueue {
    int size, max_size;

    cairo_bo_event_t **elements;
    cairo_bo_event_t *elements_embedded[1024];
} pqueue_t;

typedef struct _cairo_bo_event_queue {
    cairo_freepool_t pool;
    pqueue_t pqueue;
    cairo_bo_event_t **start_events;
} cairo_bo_event_queue_t;

typedef struct _cairo_bo_sweep_line {
    cairo_bo_edge_t *head;
    cairo_bo_edge_t *stopped;
    int32_t current_y;
    cairo_bo_edge_t *current_edge;
} cairo_bo_sweep_line_t;

#if DEBUG_TRAPS
static void
dump_traps (cairo_traps_t *traps, const char *filename)
{
    FILE *file;
    int n;

    if (getenv ("CAIRO_DEBUG_TRAPS") == NULL)
	return;

    if (traps->has_limits) {
	printf ("%s: limits=(%d, %d, %d, %d)\n",
		filename,
		traps->limits.p1.x, traps->limits.p1.y,
		traps->limits.p2.x, traps->limits.p2.y);
    }
    printf ("%s: extents=(%d, %d, %d, %d)\n",
	    filename,
	    traps->extents.p1.x, traps->extents.p1.y,
	    traps->extents.p2.x, traps->extents.p2.y);

    file = fopen (filename, "a");
    if (file != NULL) {
	for (n = 0; n < traps->num_traps; n++) {
	    fprintf (file, "%d %d L:(%d, %d), (%d, %d) R:(%d, %d), (%d, %d)\n",
		     traps->traps[n].top,
		     traps->traps[n].bottom,
		     traps->traps[n].left.p1.x,
		     traps->traps[n].left.p1.y,
		     traps->traps[n].left.p2.x,
		     traps->traps[n].left.p2.y,
		     traps->traps[n].right.p1.x,
		     traps->traps[n].right.p1.y,
		     traps->traps[n].right.p2.x,
		     traps->traps[n].right.p2.y);
	}
	fprintf (file, "\n");
	fclose (file);
    }
}

static void
dump_edges (cairo_bo_start_event_t *events,
	    int num_edges,
	    const char *filename)
{
    FILE *file;
    int n;

    if (getenv ("CAIRO_DEBUG_TRAPS") == NULL)
	return;

    file = fopen (filename, "a");
    if (file != NULL) {
	for (n = 0; n < num_edges; n++) {
	    fprintf (file, "(%d, %d), (%d, %d) %d %d %d\n",
		     events[n].edge.edge.line.p1.x,
		     events[n].edge.edge.line.p1.y,
		     events[n].edge.edge.line.p2.x,
		     events[n].edge.edge.line.p2.y,
		     events[n].edge.edge.top,
		     events[n].edge.edge.bottom,
		     events[n].edge.edge.dir);
	}
	fprintf (file, "\n");
	fclose (file);
    }
}
#endif

static cairo_fixed_t
_line_compute_intersection_x_for_y (const cairo_line_t *line,
				    cairo_fixed_t y)
{
    cairo_fixed_t x, dy;

    if (y == line->p1.y)
	return line->p1.x;
    if (y == line->p2.y)
	return line->p2.x;

    x = line->p1.x;
    dy = line->p2.y - line->p1.y;
    if (dy != 0) {
	x += _cairo_fixed_mul_div_floor (y - line->p1.y,
					 line->p2.x - line->p1.x,
					 dy);
    }

    return x;
}

static inline int
_cairo_bo_point32_compare (cairo_bo_point32_t const *a,
			   cairo_bo_point32_t const *b)
{
    int cmp;

    cmp = a->y - b->y;
    if (cmp)
	return cmp;

    return a->x - b->x;
}



































static inline int
_slope_compare (const cairo_bo_edge_t *a,
		const cairo_bo_edge_t *b)
{
    




    int32_t adx = a->edge.line.p2.x - a->edge.line.p1.x;
    int32_t bdx = b->edge.line.p2.x - b->edge.line.p1.x;

    



    
    if (adx == 0)
	return -bdx;
    if (bdx == 0)
	return adx;

    
    if ((adx ^ bdx) < 0)
	return adx;

    
    {
	int32_t ady = a->edge.line.p2.y - a->edge.line.p1.y;
	int32_t bdy = b->edge.line.p2.y - b->edge.line.p1.y;
	cairo_int64_t adx_bdy = _cairo_int32x32_64_mul (adx, bdy);
	cairo_int64_t bdx_ady = _cairo_int32x32_64_mul (bdx, ady);

	return _cairo_int64_cmp (adx_bdy, bdx_ady);
    }
}



























static int
edges_compare_x_for_y_general (const cairo_bo_edge_t *a,
			       const cairo_bo_edge_t *b,
			       int32_t y)
{
    




    int32_t dx;
    int32_t adx, ady;
    int32_t bdx, bdy;
    enum {
       HAVE_NONE    = 0x0,
       HAVE_DX      = 0x1,
       HAVE_ADX     = 0x2,
       HAVE_DX_ADX  = HAVE_DX | HAVE_ADX,
       HAVE_BDX     = 0x4,
       HAVE_DX_BDX  = HAVE_DX | HAVE_BDX,
       HAVE_ADX_BDX = HAVE_ADX | HAVE_BDX,
       HAVE_ALL     = HAVE_DX | HAVE_ADX | HAVE_BDX
    } have_dx_adx_bdx = HAVE_ALL;

    

    {
           int32_t amin, amax;
           int32_t bmin, bmax;
           if (a->edge.line.p1.x < a->edge.line.p2.x) {
                   amin = a->edge.line.p1.x;
                   amax = a->edge.line.p2.x;
           } else {
                   amin = a->edge.line.p2.x;
                   amax = a->edge.line.p1.x;
           }
           if (b->edge.line.p1.x < b->edge.line.p2.x) {
                   bmin = b->edge.line.p1.x;
                   bmax = b->edge.line.p2.x;
           } else {
                   bmin = b->edge.line.p2.x;
                   bmax = b->edge.line.p1.x;
           }
           if (amax < bmin) return -1;
           if (amin > bmax) return +1;
    }

    ady = a->edge.line.p2.y - a->edge.line.p1.y;
    adx = a->edge.line.p2.x - a->edge.line.p1.x;
    if (adx == 0)
	have_dx_adx_bdx &= ~HAVE_ADX;

    bdy = b->edge.line.p2.y - b->edge.line.p1.y;
    bdx = b->edge.line.p2.x - b->edge.line.p1.x;
    if (bdx == 0)
	have_dx_adx_bdx &= ~HAVE_BDX;

    dx = a->edge.line.p1.x - b->edge.line.p1.x;
    if (dx == 0)
	have_dx_adx_bdx &= ~HAVE_DX;

#define L _cairo_int64x32_128_mul (_cairo_int32x32_64_mul (ady, bdy), dx)
#define A _cairo_int64x32_128_mul (_cairo_int32x32_64_mul (adx, bdy), y - a->edge.line.p1.y)
#define B _cairo_int64x32_128_mul (_cairo_int32x32_64_mul (bdx, ady), y - b->edge.line.p1.y)
    switch (have_dx_adx_bdx) {
    default:
    case HAVE_NONE:
	return 0;
    case HAVE_DX:
	
	return dx; 
    case HAVE_ADX:
	
	return adx; 
    case HAVE_BDX:
	
	return -bdx; 
    case HAVE_ADX_BDX:
	
	if ((adx ^ bdx) < 0) {
	    return adx;
	} else if (a->edge.line.p1.y == b->edge.line.p1.y) { 
	    cairo_int64_t adx_bdy, bdx_ady;

	    

	    adx_bdy = _cairo_int32x32_64_mul (adx, bdy);
	    bdx_ady = _cairo_int32x32_64_mul (bdx, ady);

	    return _cairo_int64_cmp (adx_bdy, bdx_ady);
	} else
	    return _cairo_int128_cmp (A, B);
    case HAVE_DX_ADX:
	
	if ((-adx ^ dx) < 0) {
	    return dx;
	} else {
	    cairo_int64_t ady_dx, dy_adx;

	    ady_dx = _cairo_int32x32_64_mul (ady, dx);
	    dy_adx = _cairo_int32x32_64_mul (a->edge.line.p1.y - y, adx);

	    return _cairo_int64_cmp (ady_dx, dy_adx);
	}
    case HAVE_DX_BDX:
	
	if ((bdx ^ dx) < 0) {
	    return dx;
	} else {
	    cairo_int64_t bdy_dx, dy_bdx;

	    bdy_dx = _cairo_int32x32_64_mul (bdy, dx);
	    dy_bdx = _cairo_int32x32_64_mul (y - b->edge.line.p1.y, bdx);

	    return _cairo_int64_cmp (bdy_dx, dy_bdx);
	}
    case HAVE_ALL:
	
	return _cairo_int128_cmp (L, _cairo_int128_sub (B, A));
    }
#undef B
#undef A
#undef L
}






















static int
edge_compare_for_y_against_x (const cairo_bo_edge_t *a,
			      int32_t y,
			      int32_t x)
{
    int32_t adx, ady;
    int32_t dx, dy;
    cairo_int64_t L, R;

    if (x < a->edge.line.p1.x && x < a->edge.line.p2.x)
	return 1;
    if (x > a->edge.line.p1.x && x > a->edge.line.p2.x)
	return -1;

    adx = a->edge.line.p2.x - a->edge.line.p1.x;
    dx = x - a->edge.line.p1.x;

    if (adx == 0)
	return -dx;
    if (dx == 0 || (adx ^ dx) < 0)
	return adx;

    dy = y - a->edge.line.p1.y;
    ady = a->edge.line.p2.y - a->edge.line.p1.y;

    L = _cairo_int32x32_64_mul (dy, adx);
    R = _cairo_int32x32_64_mul (dx, ady);

    return _cairo_int64_cmp (L, R);
}

static int
edges_compare_x_for_y (const cairo_bo_edge_t *a,
		       const cairo_bo_edge_t *b,
		       int32_t y)
{
    




    enum {
       HAVE_NEITHER = 0x0,
       HAVE_AX      = 0x1,
       HAVE_BX      = 0x2,
       HAVE_BOTH    = HAVE_AX | HAVE_BX
    } have_ax_bx = HAVE_BOTH;
    int32_t ax, bx;

    if (y == a->edge.line.p1.y)
	ax = a->edge.line.p1.x;
    else if (y == a->edge.line.p2.y)
	ax = a->edge.line.p2.x;
    else
	have_ax_bx &= ~HAVE_AX;

    if (y == b->edge.line.p1.y)
	bx = b->edge.line.p1.x;
    else if (y == b->edge.line.p2.y)
	bx = b->edge.line.p2.x;
    else
	have_ax_bx &= ~HAVE_BX;

    switch (have_ax_bx) {
    default:
    case HAVE_NEITHER:
	return edges_compare_x_for_y_general (a, b, y);
    case HAVE_AX:
	return -edge_compare_for_y_against_x (b, y, ax);
    case HAVE_BX:
	return edge_compare_for_y_against_x (a, y, bx);
    case HAVE_BOTH:
	return ax - bx;
    }
}

static inline int
_line_equal (const cairo_line_t *a, const cairo_line_t *b)
{
    return a->p1.x == b->p1.x && a->p1.y == b->p1.y &&
           a->p2.x == b->p2.x && a->p2.y == b->p2.y;
}

static int
_cairo_bo_sweep_line_compare_edges (cairo_bo_sweep_line_t	*sweep_line,
				    const cairo_bo_edge_t	*a,
				    const cairo_bo_edge_t	*b)
{
    int cmp;

    
    if (! _line_equal (&a->edge.line, &b->edge.line)) {
	cmp = edges_compare_x_for_y (a, b, sweep_line->current_y);
	if (cmp)
	    return cmp;

	




	cmp = _slope_compare (a, b);
	if (cmp)
	    return cmp;
    }

    
    return b->edge.bottom - a->edge.bottom;
}

static inline cairo_int64_t
det32_64 (int32_t a, int32_t b,
	  int32_t c, int32_t d)
{
    
    return _cairo_int64_sub (_cairo_int32x32_64_mul (a, d),
			     _cairo_int32x32_64_mul (b, c));
}

static inline cairo_int128_t
det64x32_128 (cairo_int64_t a, int32_t       b,
	      cairo_int64_t c, int32_t       d)
{
    
    return _cairo_int128_sub (_cairo_int64x32_128_mul (a, d),
			      _cairo_int64x32_128_mul (c, b));
}







static cairo_bool_t
intersect_lines (cairo_bo_edge_t		*a,
		 cairo_bo_edge_t		*b,
		 cairo_bo_intersect_point_t	*intersection)
{
    cairo_int64_t a_det, b_det;

    





    int32_t dx1 = a->edge.line.p1.x - a->edge.line.p2.x;
    int32_t dy1 = a->edge.line.p1.y - a->edge.line.p2.y;

    int32_t dx2 = b->edge.line.p1.x - b->edge.line.p2.x;
    int32_t dy2 = b->edge.line.p1.y - b->edge.line.p2.y;

    cairo_int64_t den_det;
    cairo_int64_t R;
    cairo_quorem64_t qr;

    den_det = det32_64 (dx1, dy1, dx2, dy2);

     


















    R = det32_64 (dx2, dy2,
		  b->edge.line.p1.x - a->edge.line.p1.x,
		  b->edge.line.p1.y - a->edge.line.p1.y);
    if (_cairo_int64_negative (den_det)) {
	if (_cairo_int64_ge (den_det, R))
	    return FALSE;
    } else {
	if (_cairo_int64_le (den_det, R))
	    return FALSE;
    }

    R = det32_64 (dy1, dx1,
		  a->edge.line.p1.y - b->edge.line.p1.y,
		  a->edge.line.p1.x - b->edge.line.p1.x);
    if (_cairo_int64_negative (den_det)) {
	if (_cairo_int64_ge (den_det, R))
	    return FALSE;
    } else {
	if (_cairo_int64_le (den_det, R))
	    return FALSE;
    }

    

    a_det = det32_64 (a->edge.line.p1.x, a->edge.line.p1.y,
		      a->edge.line.p2.x, a->edge.line.p2.y);
    b_det = det32_64 (b->edge.line.p1.x, b->edge.line.p1.y,
		      b->edge.line.p2.x, b->edge.line.p2.y);

    
    qr = _cairo_int_96by64_32x64_divrem (det64x32_128 (a_det, dx1,
						       b_det, dx2),
					 den_det);
    if (_cairo_int64_eq (qr.rem, den_det))
	return FALSE;
#if 0
    intersection->x.exactness = _cairo_int64_is_zero (qr.rem) ? EXACT : INEXACT;
#else
    intersection->x.exactness = EXACT;
    if (! _cairo_int64_is_zero (qr.rem)) {
	if (_cairo_int64_negative (den_det) ^ _cairo_int64_negative (qr.rem))
	    qr.rem = _cairo_int64_negate (qr.rem);
	qr.rem = _cairo_int64_mul (qr.rem, _cairo_int32_to_int64 (2));
	if (_cairo_int64_ge (qr.rem, den_det)) {
	    qr.quo = _cairo_int64_add (qr.quo,
				       _cairo_int32_to_int64 (_cairo_int64_negative (qr.quo) ? -1 : 1));
	} else
	    intersection->x.exactness = INEXACT;
    }
#endif
    intersection->x.ordinate = _cairo_int64_to_int32 (qr.quo);

    
    qr = _cairo_int_96by64_32x64_divrem (det64x32_128 (a_det, dy1,
						       b_det, dy2),
					 den_det);
    if (_cairo_int64_eq (qr.rem, den_det))
	return FALSE;
#if 0
    intersection->y.exactness = _cairo_int64_is_zero (qr.rem) ? EXACT : INEXACT;
#else
    intersection->y.exactness = EXACT;
    if (! _cairo_int64_is_zero (qr.rem)) {
	if (_cairo_int64_negative (den_det) ^ _cairo_int64_negative (qr.rem))
	    qr.rem = _cairo_int64_negate (qr.rem);
	qr.rem = _cairo_int64_mul (qr.rem, _cairo_int32_to_int64 (2));
	if (_cairo_int64_ge (qr.rem, den_det)) {
	    qr.quo = _cairo_int64_add (qr.quo,
				       _cairo_int32_to_int64 (_cairo_int64_negative (qr.quo) ? -1 : 1));
	} else
	    intersection->y.exactness = INEXACT;
    }
#endif
    intersection->y.ordinate = _cairo_int64_to_int32 (qr.quo);

    return TRUE;
}

static int
_cairo_bo_intersect_ordinate_32_compare (cairo_bo_intersect_ordinate_t	a,
					 int32_t			b)
{
    
    if (a.ordinate > b)
	return +1;
    if (a.ordinate < b)
	return -1;
    
    
    return INEXACT == a.exactness;
}


















static cairo_bool_t
_cairo_bo_edge_contains_intersect_point (cairo_bo_edge_t		*edge,
					 cairo_bo_intersect_point_t	*point)
{
    int cmp_top, cmp_bottom;

    






    cmp_top = _cairo_bo_intersect_ordinate_32_compare (point->y,
						       edge->edge.top);
    cmp_bottom = _cairo_bo_intersect_ordinate_32_compare (point->y,
							  edge->edge.bottom);

    if (cmp_top < 0 || cmp_bottom > 0)
    {
	return FALSE;
    }

    if (cmp_top > 0 && cmp_bottom < 0)
    {
	return TRUE;
    }

    



    






    if (cmp_top == 0) {
	cairo_fixed_t top_x;

	top_x = _line_compute_intersection_x_for_y (&edge->edge.line,
						    edge->edge.top);
	return _cairo_bo_intersect_ordinate_32_compare (point->x, top_x) > 0;
    } else { 
	cairo_fixed_t bot_x;

	bot_x = _line_compute_intersection_x_for_y (&edge->edge.line,
						    edge->edge.bottom);
	return _cairo_bo_intersect_ordinate_32_compare (point->x, bot_x) < 0;
    }
}

















static cairo_bool_t
_cairo_bo_edge_intersect (cairo_bo_edge_t	*a,
			  cairo_bo_edge_t	*b,
			  cairo_bo_point32_t	*intersection)
{
    cairo_bo_intersect_point_t quorem;

    if (! intersect_lines (a, b, &quorem))
	return FALSE;

    if (! _cairo_bo_edge_contains_intersect_point (a, &quorem))
	return FALSE;

    if (! _cairo_bo_edge_contains_intersect_point (b, &quorem))
	return FALSE;

    




    intersection->x = quorem.x.ordinate;
    intersection->y = quorem.y.ordinate;

    return TRUE;
}

static inline int
cairo_bo_event_compare (const cairo_bo_event_t *a,
			const cairo_bo_event_t *b)
{
    int cmp;

    cmp = _cairo_bo_point32_compare (&a->point, &b->point);
    if (cmp)
	return cmp;

    cmp = a->type - b->type;
    if (cmp)
	return cmp;

    return a - b;
}

static inline void
_pqueue_init (pqueue_t *pq)
{
    pq->max_size = ARRAY_LENGTH (pq->elements_embedded);
    pq->size = 0;

    pq->elements = pq->elements_embedded;
}

static inline void
_pqueue_fini (pqueue_t *pq)
{
    if (pq->elements != pq->elements_embedded)
	free (pq->elements);
}

static cairo_status_t
_pqueue_grow (pqueue_t *pq)
{
    cairo_bo_event_t **new_elements;
    pq->max_size *= 2;

    if (pq->elements == pq->elements_embedded) {
	new_elements = _cairo_malloc_ab (pq->max_size,
					 sizeof (cairo_bo_event_t *));
	if (unlikely (new_elements == NULL))
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	memcpy (new_elements, pq->elements_embedded,
		sizeof (pq->elements_embedded));
    } else {
	new_elements = _cairo_realloc_ab (pq->elements,
					  pq->max_size,
					  sizeof (cairo_bo_event_t *));
	if (unlikely (new_elements == NULL))
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }

    pq->elements = new_elements;
    return CAIRO_STATUS_SUCCESS;
}

static inline cairo_status_t
_pqueue_push (pqueue_t *pq, cairo_bo_event_t *event)
{
    cairo_bo_event_t **elements;
    int i, parent;

    if (unlikely (pq->size + 1 == pq->max_size)) {
	cairo_status_t status;

	status = _pqueue_grow (pq);
	if (unlikely (status))
	    return status;
    }

    elements = pq->elements;

    for (i = ++pq->size;
	 i != PQ_FIRST_ENTRY &&
	 cairo_bo_event_compare (event,
				 elements[parent = PQ_PARENT_INDEX (i)]) < 0;
	 i = parent)
    {
	elements[i] = elements[parent];
    }

    elements[i] = event;

    return CAIRO_STATUS_SUCCESS;
}

static inline void
_pqueue_pop (pqueue_t *pq)
{
    cairo_bo_event_t **elements = pq->elements;
    cairo_bo_event_t *tail;
    int child, i;

    tail = elements[pq->size--];
    if (pq->size == 0) {
	elements[PQ_FIRST_ENTRY] = NULL;
	return;
    }

    for (i = PQ_FIRST_ENTRY;
	 (child = PQ_LEFT_CHILD_INDEX (i)) <= pq->size;
	 i = child)
    {
	if (child != pq->size &&
	    cairo_bo_event_compare (elements[child+1],
				    elements[child]) < 0)
	{
	    child++;
	}

	if (cairo_bo_event_compare (elements[child], tail) >= 0)
	    break;

	elements[i] = elements[child];
    }
    elements[i] = tail;
}

static inline cairo_status_t
_cairo_bo_event_queue_insert (cairo_bo_event_queue_t	*queue,
			      cairo_bo_event_type_t	 type,
			      cairo_bo_edge_t		*e1,
			      cairo_bo_edge_t		*e2,
			      const cairo_point_t	 *point)
{
    cairo_bo_queue_event_t *event;

    event = _cairo_freepool_alloc (&queue->pool);
    if (unlikely (event == NULL))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    event->type = type;
    event->e1 = e1;
    event->e2 = e2;
    event->point = *point;

    return _pqueue_push (&queue->pqueue, (cairo_bo_event_t *) event);
}

static void
_cairo_bo_event_queue_delete (cairo_bo_event_queue_t *queue,
			      cairo_bo_event_t	     *event)
{
    _cairo_freepool_free (&queue->pool, event);
}

static cairo_bo_event_t *
_cairo_bo_event_dequeue (cairo_bo_event_queue_t *event_queue)
{
    cairo_bo_event_t *event, *cmp;

    event = event_queue->pqueue.elements[PQ_FIRST_ENTRY];
    cmp = *event_queue->start_events;
    if (event == NULL ||
	(cmp != NULL && cairo_bo_event_compare (cmp, event) < 0))
    {
	event = cmp;
	event_queue->start_events++;
    }
    else
    {
	_pqueue_pop (&event_queue->pqueue);
    }

    return event;
}

CAIRO_COMBSORT_DECLARE (_cairo_bo_event_queue_sort,
			cairo_bo_event_t *,
			cairo_bo_event_compare)

static void
_cairo_bo_event_queue_init (cairo_bo_event_queue_t	 *event_queue,
			    cairo_bo_event_t		**start_events,
			    int				  num_events)
{
    _cairo_bo_event_queue_sort (start_events, num_events);
    start_events[num_events] = NULL;

    event_queue->start_events = start_events;

    _cairo_freepool_init (&event_queue->pool,
			  sizeof (cairo_bo_queue_event_t));
    _pqueue_init (&event_queue->pqueue);
    event_queue->pqueue.elements[PQ_FIRST_ENTRY] = NULL;
}

static cairo_status_t
_cairo_bo_event_queue_insert_stop (cairo_bo_event_queue_t	*event_queue,
				   cairo_bo_edge_t		*edge)
{
    cairo_bo_point32_t point;

    point.y = edge->edge.bottom;
    point.x = _line_compute_intersection_x_for_y (&edge->edge.line,
						  point.y);
    return _cairo_bo_event_queue_insert (event_queue,
					 CAIRO_BO_EVENT_TYPE_STOP,
					 edge, NULL,
					 &point);
}

static void
_cairo_bo_event_queue_fini (cairo_bo_event_queue_t *event_queue)
{
    _pqueue_fini (&event_queue->pqueue);
    _cairo_freepool_fini (&event_queue->pool);
}

static inline cairo_status_t
_cairo_bo_event_queue_insert_if_intersect_below_current_y (cairo_bo_event_queue_t	*event_queue,
							   cairo_bo_edge_t	*left,
							   cairo_bo_edge_t *right)
{
    cairo_bo_point32_t intersection;

    if (_line_equal (&left->edge.line, &right->edge.line))
	return CAIRO_STATUS_SUCCESS;

    




    if (_slope_compare (left, right) <= 0)
	return CAIRO_STATUS_SUCCESS;

    if (! _cairo_bo_edge_intersect (left, right, &intersection))
	return CAIRO_STATUS_SUCCESS;

    return _cairo_bo_event_queue_insert (event_queue,
					 CAIRO_BO_EVENT_TYPE_INTERSECTION,
					 left, right,
					 &intersection);
}

static void
_cairo_bo_sweep_line_init (cairo_bo_sweep_line_t *sweep_line)
{
    sweep_line->head = NULL;
    sweep_line->stopped = NULL;
    sweep_line->current_y = INT32_MIN;
    sweep_line->current_edge = NULL;
}

static cairo_status_t
_cairo_bo_sweep_line_insert (cairo_bo_sweep_line_t	*sweep_line,
			     cairo_bo_edge_t		*edge)
{
    if (sweep_line->current_edge != NULL) {
	cairo_bo_edge_t *prev, *next;
	int cmp;

	cmp = _cairo_bo_sweep_line_compare_edges (sweep_line,
						  sweep_line->current_edge,
						  edge);
	if (cmp < 0) {
	    prev = sweep_line->current_edge;
	    next = prev->next;
	    while (next != NULL &&
		   _cairo_bo_sweep_line_compare_edges (sweep_line,
						       next, edge) < 0)
	    {
		prev = next, next = prev->next;
	    }

	    prev->next = edge;
	    edge->prev = prev;
	    edge->next = next;
	    if (next != NULL)
		next->prev = edge;
	} else if (cmp > 0) {
	    next = sweep_line->current_edge;
	    prev = next->prev;
	    while (prev != NULL &&
		   _cairo_bo_sweep_line_compare_edges (sweep_line,
						       prev, edge) > 0)
	    {
		next = prev, prev = next->prev;
	    }

	    next->prev = edge;
	    edge->next = next;
	    edge->prev = prev;
	    if (prev != NULL)
		prev->next = edge;
	    else
		sweep_line->head = edge;
	} else {
	    prev = sweep_line->current_edge;
	    edge->prev = prev;
	    edge->next = prev->next;
	    if (prev->next != NULL)
		prev->next->prev = edge;
	    prev->next = edge;
	}
    } else {
	sweep_line->head = edge;
    }

    sweep_line->current_edge = edge;

    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_bo_sweep_line_delete (cairo_bo_sweep_line_t	*sweep_line,
			     cairo_bo_edge_t	*edge)
{
    if (edge->prev != NULL)
	edge->prev->next = edge->next;
    else
	sweep_line->head = edge->next;

    if (edge->next != NULL)
	edge->next->prev = edge->prev;

    if (sweep_line->current_edge == edge)
	sweep_line->current_edge = edge->prev ? edge->prev : edge->next;
}

static void
_cairo_bo_sweep_line_swap (cairo_bo_sweep_line_t	*sweep_line,
			   cairo_bo_edge_t		*left,
			   cairo_bo_edge_t		*right)
{
    if (left->prev != NULL)
	left->prev->next = right;
    else
	sweep_line->head = right;

    if (right->next != NULL)
	right->next->prev = left;

    left->next = right->next;
    right->next = left;

    right->prev = left->prev;
    left->prev = right;
}

#if DEBUG_PRINT_STATE
static void
_cairo_bo_edge_print (cairo_bo_edge_t *edge)
{
    printf ("(0x%x, 0x%x)-(0x%x, 0x%x)",
	    edge->edge.line.p1.x, edge->edge.line.p1.y,
	    edge->edge.line.p2.x, edge->edge.line.p2.y);
}

static void
_cairo_bo_event_print (cairo_bo_event_t *event)
{
    switch (event->type) {
    case CAIRO_BO_EVENT_TYPE_START:
	printf ("Start: ");
	break;
    case CAIRO_BO_EVENT_TYPE_STOP:
	printf ("Stop: ");
	break;
    case CAIRO_BO_EVENT_TYPE_INTERSECTION:
	printf ("Intersection: ");
	break;
    }
    printf ("(%d, %d)\t", event->point.x, event->point.y);
    _cairo_bo_edge_print (event->e1);
    if (event->type == CAIRO_BO_EVENT_TYPE_INTERSECTION) {
	printf (" X ");
	_cairo_bo_edge_print (event->e2);
    }
    printf ("\n");
}

static void
_cairo_bo_event_queue_print (cairo_bo_event_queue_t *event_queue)
{
    
    printf ("Event queue:\n");
}

static void
_cairo_bo_sweep_line_print (cairo_bo_sweep_line_t *sweep_line)
{
    cairo_bool_t first = TRUE;
    cairo_bo_edge_t *edge;

    printf ("Sweep line from edge list: ");
    first = TRUE;
    for (edge = sweep_line->head;
	 edge;
	 edge = edge->next)
    {
	if (!first)
	    printf (", ");
	_cairo_bo_edge_print (edge);
	first = FALSE;
    }
    printf ("\n");
}

static void
print_state (const char			*msg,
	     cairo_bo_event_t		*event,
	     cairo_bo_event_queue_t	*event_queue,
	     cairo_bo_sweep_line_t	*sweep_line)
{
    printf ("%s ", msg);
    _cairo_bo_event_print (event);
    _cairo_bo_event_queue_print (event_queue);
    _cairo_bo_sweep_line_print (sweep_line);
    printf ("\n");
}
#endif

#if DEBUG_EVENTS
static void CAIRO_PRINTF_FORMAT (1, 2)
event_log (const char *fmt, ...)
{
    FILE *file;

    if (getenv ("CAIRO_DEBUG_EVENTS") == NULL)
	return;

    file = fopen ("bo-events.txt", "a");
    if (file != NULL) {
	va_list ap;

	va_start (ap, fmt);
	vfprintf (file, fmt, ap);
	va_end (ap);

	fclose (file);
    }
}
#endif

static inline cairo_bool_t
edges_colinear (const cairo_bo_edge_t *a, const cairo_bo_edge_t *b)
{
    if (_line_equal (&a->edge.line, &b->edge.line))
	return TRUE;

    if (_slope_compare (a, b))
	return FALSE;

    


    if (a->edge.line.p1.y == b->edge.line.p1.y) {
	return a->edge.line.p1.x == b->edge.line.p1.x;
    } else if (a->edge.line.p1.y < b->edge.line.p1.y) {
	return edge_compare_for_y_against_x (b,
					     a->edge.line.p1.y,
					     a->edge.line.p1.x) == 0;
    } else {
	return edge_compare_for_y_against_x (a,
					     b->edge.line.p1.y,
					     b->edge.line.p1.x) == 0;
    }
}


static cairo_status_t
_cairo_bo_edge_end_trap (cairo_bo_edge_t	*left,
			 int32_t		 bot,
			 cairo_traps_t	        *traps)
{
    cairo_bo_trap_t *trap = &left->deferred_trap;

    
    if (likely (trap->top < bot)) {
	_cairo_traps_add_trap (traps,
			       trap->top, bot,
			       &left->edge.line, &trap->right->edge.line);

#if DEBUG_PRINT_STATE
	printf ("Deferred trap: left=(%x, %x)-(%x,%x) "
		"right=(%x,%x)-(%x,%x) top=%x, bot=%x\n",
		left->edge.line.p1.x, left->edge.line.p1.y,
		left->edge.line.p2.x, left->edge.line.p2.y,
		trap->right->edge.line.p1.x, trap->right->edge.line.p1.y,
		trap->right->edge.line.p2.x, trap->right->edge.line.p2.y,
		trap->top, bot);
#endif
#if DEBUG_EVENTS
	event_log ("end trap: %lu %lu %d %d\n",
		   (long) left,
		   (long) trap->right,
		   trap->top,
		   bot);
#endif
    }

    trap->right = NULL;

    return _cairo_traps_status (traps);
}







static inline cairo_status_t
_cairo_bo_edge_start_or_continue_trap (cairo_bo_edge_t	*left,
				       cairo_bo_edge_t  *right,
				       int               top,
				       cairo_traps_t	*traps)
{
    cairo_status_t status;

    if (left->deferred_trap.right == right)
	return CAIRO_STATUS_SUCCESS;

    if (left->deferred_trap.right != NULL) {
	if (right != NULL && edges_colinear (left->deferred_trap.right, right))
	{
	    
	    left->deferred_trap.right = right;
	    return CAIRO_STATUS_SUCCESS;
	}

	status = _cairo_bo_edge_end_trap (left, top, traps);
	if (unlikely (status))
	    return status;
    }

    if (right != NULL && ! edges_colinear (left, right)) {
	left->deferred_trap.top = top;
	left->deferred_trap.right = right;

#if DEBUG_EVENTS
	event_log ("begin trap: %lu %lu %d\n",
		   (long) left,
		   (long) right,
		   top);
#endif
    }

    return CAIRO_STATUS_SUCCESS;
}

static inline cairo_status_t
_active_edges_to_traps (cairo_bo_edge_t		*left,
			int32_t			 top,
			cairo_fill_rule_t	 fill_rule,
			cairo_traps_t	        *traps)
{
    cairo_bo_edge_t *right;
    cairo_status_t status;

#if DEBUG_PRINT_STATE
    printf ("Processing active edges for %x\n", top);
#endif

    if (fill_rule == CAIRO_FILL_RULE_WINDING) {
	while (left != NULL) {
	    int in_out;

	    


	    in_out = left->edge.dir;

	    
	    right = left->next;
	    if (left->deferred_trap.right == NULL) {
		while (right != NULL && right->deferred_trap.right == NULL)
		    right = right->next;

		if (right != NULL && edges_colinear (left, right)) {
		    
		    left->deferred_trap = right->deferred_trap;
		    right->deferred_trap.right = NULL;
		}
	    }

	    
	    right = left->next;
	    while (right != NULL) {
		if (right->deferred_trap.right != NULL) {
		    status = _cairo_bo_edge_end_trap (right, top, traps);
		    if (unlikely (status))
			return status;
		}

		in_out += right->edge.dir;
		if (in_out == 0) {
		    cairo_bo_edge_t *next;
		    cairo_bool_t skip = FALSE;

		    
		    next = right->next;
		    if (next != NULL)
			skip = edges_colinear (right, next);

		    if (! skip)
			break;
		}

		right = right->next;
	    }

	    status = _cairo_bo_edge_start_or_continue_trap (left, right,
							    top, traps);
	    if (unlikely (status))
		return status;

	    left = right;
	    if (left != NULL)
		left = left->next;
	}
    } else {
	while (left != NULL) {
	    int in_out = 0;

	    right = left->next;
	    while (right != NULL) {
		if (right->deferred_trap.right != NULL) {
		    status = _cairo_bo_edge_end_trap (right, top, traps);
		    if (unlikely (status))
			return status;
		}

		if ((in_out++ & 1) == 0) {
		    cairo_bo_edge_t *next;
		    cairo_bool_t skip = FALSE;

		    
		    next = right->next;
		    if (next != NULL)
			skip = edges_colinear (right, next);

		    if (! skip)
			break;
		}

		right = right->next;
	    }

	    status = _cairo_bo_edge_start_or_continue_trap (left, right,
							    top, traps);
	    if (unlikely (status))
		return status;

	    left = right;
	    if (left != NULL)
		left = left->next;
	}
    }

    return CAIRO_STATUS_SUCCESS;
}





static cairo_status_t
_cairo_bentley_ottmann_tessellate_bo_edges (cairo_bo_event_t   **start_events,
					    int			 num_events,
					    cairo_fill_rule_t	 fill_rule,
					    cairo_traps_t	*traps,
					    int			*num_intersections)
{
    cairo_status_t status = CAIRO_STATUS_SUCCESS; 
    int intersection_count = 0;
    cairo_bo_event_queue_t event_queue;
    cairo_bo_sweep_line_t sweep_line;
    cairo_bo_event_t *event;
    cairo_bo_edge_t *left, *right;
    cairo_bo_edge_t *e1, *e2;

#if DEBUG_EVENTS
    {
	int i;

	for (i = 0; i < num_events; i++) {
	    cairo_bo_start_event_t *event =
		((cairo_bo_start_event_t **) start_events)[i];
	    event_log ("edge: %lu (%d, %d) (%d, %d) (%d, %d) %d\n",
		       (long) &events[i].edge,
		       event->edge.edge.line.p1.x,
		       event->edge.edge.line.p1.y,
		       event->edge.edge.line.p2.x,
		       event->edge.edge.line.p2.y,
		       event->edge.top,
		       event->edge.bottom,
		       event->edge.edge.dir);
	}
    }
#endif

    _cairo_bo_event_queue_init (&event_queue, start_events, num_events);
    _cairo_bo_sweep_line_init (&sweep_line);

    while ((event = _cairo_bo_event_dequeue (&event_queue))) {
	if (event->point.y != sweep_line.current_y) {
	    for (e1 = sweep_line.stopped; e1; e1 = e1->next) {
		if (e1->deferred_trap.right != NULL) {
		    status = _cairo_bo_edge_end_trap (e1,
						      e1->edge.bottom,
						      traps);
		    if (unlikely (status))
			goto unwind;
		}
	    }
	    sweep_line.stopped = NULL;

	    status = _active_edges_to_traps (sweep_line.head,
					     sweep_line.current_y,
					     fill_rule, traps);
	    if (unlikely (status))
		goto unwind;

	    sweep_line.current_y = event->point.y;
	}

#if DEBUG_EVENTS
	event_log ("event: %d (%ld, %ld) %lu, %lu\n",
		   event->type,
		   (long) event->point.x,
		   (long) event->point.y,
		   (long) event->e1,
		   (long) event->e2);
#endif

	switch (event->type) {
	case CAIRO_BO_EVENT_TYPE_START:
	    e1 = &((cairo_bo_start_event_t *) event)->edge;

	    status = _cairo_bo_sweep_line_insert (&sweep_line, e1);
	    if (unlikely (status))
		goto unwind;

	    status = _cairo_bo_event_queue_insert_stop (&event_queue, e1);
	    if (unlikely (status))
		goto unwind;

	    
	    
	    for (left = sweep_line.stopped; left; left = left->next) {
		if (e1->edge.top <= left->edge.bottom &&
		    edges_colinear (e1, left))
		{
		    e1->deferred_trap = left->deferred_trap;
		    if (left->prev != NULL)
			left->prev = left->next;
		    else
			sweep_line.stopped = left->next;
		    if (left->next != NULL)
			left->next->prev = left->prev;
		    break;
		}
	    }

	    left = e1->prev;
	    right = e1->next;

	    if (left != NULL) {
		status = _cairo_bo_event_queue_insert_if_intersect_below_current_y (&event_queue, left, e1);
		if (unlikely (status))
		    goto unwind;
	    }

	    if (right != NULL) {
		status = _cairo_bo_event_queue_insert_if_intersect_below_current_y (&event_queue, e1, right);
		if (unlikely (status))
		    goto unwind;
	    }

	    break;

	case CAIRO_BO_EVENT_TYPE_STOP:
	    e1 = ((cairo_bo_queue_event_t *) event)->e1;
	    _cairo_bo_event_queue_delete (&event_queue, event);

	    left = e1->prev;
	    right = e1->next;

	    _cairo_bo_sweep_line_delete (&sweep_line, e1);

	    
	    if (e1->deferred_trap.right != NULL) {
		e1->next = sweep_line.stopped;
		if (sweep_line.stopped != NULL)
		    sweep_line.stopped->prev = e1;
		sweep_line.stopped = e1;
		e1->prev = NULL;
	    }

	    if (left != NULL && right != NULL) {
		status = _cairo_bo_event_queue_insert_if_intersect_below_current_y (&event_queue, left, right);
		if (unlikely (status))
		    goto unwind;
	    }

	    break;

	case CAIRO_BO_EVENT_TYPE_INTERSECTION:
	    e1 = ((cairo_bo_queue_event_t *) event)->e1;
	    e2 = ((cairo_bo_queue_event_t *) event)->e2;
	    _cairo_bo_event_queue_delete (&event_queue, event);

	    
	    if (e2 != e1->next)
		break;

	    intersection_count++;

	    left = e1->prev;
	    right = e2->next;

	    _cairo_bo_sweep_line_swap (&sweep_line, e1, e2);

	    

	    if (left != NULL) {
		status = _cairo_bo_event_queue_insert_if_intersect_below_current_y (&event_queue, left, e2);
		if (unlikely (status))
		    goto unwind;
	    }

	    if (right != NULL) {
		status = _cairo_bo_event_queue_insert_if_intersect_below_current_y (&event_queue, e1, right);
		if (unlikely (status))
		    goto unwind;
	    }

	    break;
	}
    }

    *num_intersections = intersection_count;
    for (e1 = sweep_line.stopped; e1; e1 = e1->next) {
	if (e1->deferred_trap.right != NULL) {
	    status = _cairo_bo_edge_end_trap (e1, e1->edge.bottom, traps);
	    if (unlikely (status))
		break;
	}
    }
 unwind:
    _cairo_bo_event_queue_fini (&event_queue);

#if DEBUG_EVENTS
    event_log ("\n");
#endif

    return status;
}

cairo_status_t
_cairo_bentley_ottmann_tessellate_polygon (cairo_traps_t	 *traps,
					   const cairo_polygon_t *polygon,
					   cairo_fill_rule_t	  fill_rule)
{
    int intersections;
    cairo_status_t status;
    cairo_bo_start_event_t stack_events[CAIRO_STACK_ARRAY_LENGTH (cairo_bo_start_event_t)];
    cairo_bo_start_event_t *events;
    cairo_bo_event_t *stack_event_ptrs[ARRAY_LENGTH (stack_events) + 1];
    cairo_bo_event_t **event_ptrs;
    int num_events;
    int i;

    num_events = polygon->num_edges;
    if (unlikely (0 == num_events))
	return CAIRO_STATUS_SUCCESS;

    events = stack_events;
    event_ptrs = stack_event_ptrs;
    if (num_events > ARRAY_LENGTH (stack_events)) {
	events = _cairo_malloc_ab_plus_c (num_events,
					  sizeof (cairo_bo_start_event_t) +
					  sizeof (cairo_bo_event_t *),
					  sizeof (cairo_bo_event_t *));
	if (unlikely (events == NULL))
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	event_ptrs = (cairo_bo_event_t **) (events + num_events);
    }

    for (i = 0; i < num_events; i++) {
	event_ptrs[i] = (cairo_bo_event_t *) &events[i];

	events[i].type = CAIRO_BO_EVENT_TYPE_START;
	events[i].point.y = polygon->edges[i].top;
	events[i].point.x =
	    _line_compute_intersection_x_for_y (&polygon->edges[i].line,
						events[i].point.y);

	events[i].edge.edge = polygon->edges[i];
	events[i].edge.deferred_trap.right = NULL;
	events[i].edge.prev = NULL;
	events[i].edge.next = NULL;
    }

#if DEBUG_TRAPS
    dump_edges (events, num_events, "bo-polygon-edges.txt");
#endif

    



    status = _cairo_bentley_ottmann_tessellate_bo_edges (event_ptrs,
							 num_events,
							 fill_rule, traps,
							 &intersections);
#if DEBUG_TRAPS
    dump_traps (traps, "bo-polygon-out.txt");
#endif

    if (events != stack_events)
	free (events);

    return status;
}

cairo_status_t
_cairo_bentley_ottmann_tessellate_traps (cairo_traps_t *traps,
					 cairo_fill_rule_t fill_rule)
{
    cairo_status_t status;
    cairo_polygon_t polygon;
    int i;

    if (unlikely (0 == traps->num_traps))
	return CAIRO_STATUS_SUCCESS;

#if DEBUG_TRAPS
    dump_traps (traps, "bo-traps-in.txt");
#endif

    _cairo_polygon_init (&polygon);
    _cairo_polygon_limit (&polygon, traps->limits, traps->num_limits);

    for (i = 0; i < traps->num_traps; i++) {
	status = _cairo_polygon_add_line (&polygon,
					  &traps->traps[i].left,
					  traps->traps[i].top,
					  traps->traps[i].bottom,
					  1);
	if (unlikely (status))
	    goto CLEANUP;

	status = _cairo_polygon_add_line (&polygon,
					  &traps->traps[i].right,
					  traps->traps[i].top,
					  traps->traps[i].bottom,
					  -1);
	if (unlikely (status))
	    goto CLEANUP;
    }

    _cairo_traps_clear (traps);
    status = _cairo_bentley_ottmann_tessellate_polygon (traps,
							&polygon,
							fill_rule);

#if DEBUG_TRAPS
    dump_traps (traps, "bo-traps-out.txt");
#endif

  CLEANUP:
    _cairo_polygon_fini (&polygon);

    return status;
}

#if 0
static cairo_bool_t
edges_have_an_intersection_quadratic (cairo_bo_edge_t	*edges,
				      int		 num_edges)

{
    int i, j;
    cairo_bo_edge_t *a, *b;
    cairo_bo_point32_t intersection;

    
    for (i = 0; i < num_edges; i++) {
	assert (_cairo_bo_point32_compare (&edges[i].top, &edges[i].bottom) < 0);
	edges[i].line.p1.x <<= CAIRO_BO_GUARD_BITS;
	edges[i].line.p1.y <<= CAIRO_BO_GUARD_BITS;
	edges[i].line.p2.x <<= CAIRO_BO_GUARD_BITS;
	edges[i].line.p2.y <<= CAIRO_BO_GUARD_BITS;
    }

    for (i = 0; i < num_edges; i++) {
	for (j = 0; j < num_edges; j++) {
	    if (i == j)
		continue;

	    a = &edges[i];
	    b = &edges[j];

	    if (! _cairo_bo_edge_intersect (a, b, &intersection))
		continue;

	    printf ("Found intersection (%d,%d) between (%d,%d)-(%d,%d) and (%d,%d)-(%d,%d)\n",
		    intersection.x,
		    intersection.y,
		    a->line.p1.x, a->line.p1.y,
		    a->line.p2.x, a->line.p2.y,
		    b->line.p1.x, b->line.p1.y,
		    b->line.p2.x, b->line.p2.y);

	    return TRUE;
	}
    }
    return FALSE;
}

#define TEST_MAX_EDGES 10

typedef struct test {
    const char *name;
    const char *description;
    int num_edges;
    cairo_bo_edge_t edges[TEST_MAX_EDGES];
} test_t;

static test_t
tests[] = {
    {
	"3 near misses",
	"3 edges all intersecting very close to each other",
	3,
	{
	    { { 4, 2}, {0, 0}, { 9, 9}, NULL, NULL },
	    { { 7, 2}, {0, 0}, { 2, 3}, NULL, NULL },
	    { { 5, 2}, {0, 0}, { 1, 7}, NULL, NULL }
	}
    },
    {
	"inconsistent data",
	"Derived from random testing---was leading to skip list and edge list disagreeing.",
	2,
	{
	    { { 2, 3}, {0, 0}, { 8, 9}, NULL, NULL },
	    { { 2, 3}, {0, 0}, { 6, 7}, NULL, NULL }
	}
    },
    {
	"failed sort",
	"A test derived from random testing that leads to an inconsistent sort --- looks like we just can't attempt to validate the sweep line with edge_compare?",
	3,
	{
	    { { 6, 2}, {0, 0}, { 6, 5}, NULL, NULL },
	    { { 3, 5}, {0, 0}, { 5, 6}, NULL, NULL },
	    { { 9, 2}, {0, 0}, { 5, 6}, NULL, NULL },
	}
    },
    {
	"minimal-intersection",
	"Intersection of a two from among the smallest possible edges.",
	2,
	{
	    { { 0, 0}, {0, 0}, { 1, 1}, NULL, NULL },
	    { { 1, 0}, {0, 0}, { 0, 1}, NULL, NULL }
	}
    },
    {
	"simple",
	"A simple intersection of two edges at an integer (2,2).",
	2,
	{
	    { { 1, 1}, {0, 0}, { 3, 3}, NULL, NULL },
	    { { 2, 1}, {0, 0}, { 2, 3}, NULL, NULL }
	}
    },
    {
	"bend-to-horizontal",
	"With intersection truncation one edge bends to horizontal",
	2,
	{
	    { { 9, 1}, {0, 0}, {3, 7}, NULL, NULL },
	    { { 3, 5}, {0, 0}, {9, 9}, NULL, NULL }
	}
    }
};



























































































static int
run_test (const char		*test_name,
          cairo_bo_edge_t	*test_edges,
          int			 num_edges)
{
    int i, intersections, passes;
    cairo_bo_edge_t *edges;
    cairo_array_t intersected_edges;

    printf ("Testing: %s\n", test_name);

    _cairo_array_init (&intersected_edges, sizeof (cairo_bo_edge_t));

    intersections = _cairo_bentley_ottmann_intersect_edges (test_edges, num_edges, &intersected_edges);
    if (intersections)
	printf ("Pass 1 found %d intersections:\n", intersections);


    

    passes = 1;
    while (intersections) {
	int num_edges = _cairo_array_num_elements (&intersected_edges);
	passes++;
	edges = _cairo_malloc_ab (num_edges, sizeof (cairo_bo_edge_t));
	assert (edges != NULL);
	memcpy (edges, _cairo_array_index (&intersected_edges, 0), num_edges * sizeof (cairo_bo_edge_t));
	_cairo_array_fini (&intersected_edges);
	_cairo_array_init (&intersected_edges, sizeof (cairo_bo_edge_t));
	intersections = _cairo_bentley_ottmann_intersect_edges (edges, num_edges, &intersected_edges);
	free (edges);

	if (intersections){
	    printf ("Pass %d found %d remaining intersections:\n", passes, intersections);
	} else {
	    if (passes > 3)
		for (i = 0; i < passes; i++)
		    printf ("*");
	    printf ("No remainining intersections found after pass %d\n", passes);
	}
    }

    if (edges_have_an_intersection_quadratic (_cairo_array_index (&intersected_edges, 0),
					      _cairo_array_num_elements (&intersected_edges)))
	printf ("*** FAIL ***\n");
    else
	printf ("PASS\n");

    _cairo_array_fini (&intersected_edges);

    return 0;
}

#define MAX_RANDOM 300

int
main (void)
{
    char random_name[] = "random-XX";
    cairo_bo_edge_t random_edges[MAX_RANDOM], *edge;
    unsigned int i, num_random;
    test_t *test;

    for (i = 0; i < ARRAY_LENGTH (tests); i++) {
	test = &tests[i];
	run_test (test->name, test->edges, test->num_edges);
    }

    for (num_random = 0; num_random < MAX_RANDOM; num_random++) {
	srand (0);
	for (i = 0; i < num_random; i++) {
	    do {
		edge = &random_edges[i];
		edge->line.p1.x = (int32_t) (10.0 * (rand() / (RAND_MAX + 1.0)));
		edge->line.p1.y = (int32_t) (10.0 * (rand() / (RAND_MAX + 1.0)));
		edge->line.p2.x = (int32_t) (10.0 * (rand() / (RAND_MAX + 1.0)));
		edge->line.p2.y = (int32_t) (10.0 * (rand() / (RAND_MAX + 1.0)));
		if (edge->line.p1.y > edge->line.p2.y) {
		    int32_t tmp = edge->line.p1.y;
		    edge->line.p1.y = edge->line.p2.y;
		    edge->line.p2.y = tmp;
		}
	    } while (edge->line.p1.y == edge->line.p2.y);
	}

	sprintf (random_name, "random-%02d", num_random);

	run_test (random_name, random_edges, num_random);
    }

    return 0;
}
#endif
