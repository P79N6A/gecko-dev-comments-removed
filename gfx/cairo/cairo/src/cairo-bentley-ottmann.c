



































#include "cairoint.h"

#include "cairo-skiplist-private.h"
#include "cairo-freelist-private.h"

typedef cairo_point_t cairo_bo_point32_t;

typedef struct _cairo_bo_point128 {
    cairo_int128_t x;
    cairo_int128_t y;
} cairo_bo_point128_t;

typedef struct _cairo_bo_intersect_ordinate {
    int32_t ordinate;
    enum { EXACT, INEXACT } exactness;
} cairo_bo_intersect_ordinate_t;

typedef struct _cairo_bo_intersect_point {
    cairo_bo_intersect_ordinate_t x;
    cairo_bo_intersect_ordinate_t y;
} cairo_bo_intersect_point_t;

typedef struct _cairo_bo_edge cairo_bo_edge_t;
typedef struct _sweep_line_elt sweep_line_elt_t;
typedef struct _cairo_bo_trap cairo_bo_trap_t;
typedef struct _cairo_bo_traps cairo_bo_traps_t;


struct _cairo_bo_trap {
    cairo_bo_edge_t *right;
    int32_t top;
};

struct _cairo_bo_traps {
    cairo_traps_t *traps;
    cairo_freelist_t freelist;

    

    cairo_fixed_t xmin;
    cairo_fixed_t ymin;
    cairo_fixed_t xmax;
    cairo_fixed_t ymax;
};

struct _cairo_bo_edge {
    cairo_bo_point32_t top;
    cairo_bo_point32_t middle;
    cairo_bo_point32_t bottom;
    cairo_bool_t reversed;
    cairo_bo_edge_t *prev;
    cairo_bo_edge_t *next;
    cairo_bo_trap_t *deferred_trap;
    sweep_line_elt_t *sweep_line_elt;
};

struct _sweep_line_elt {
    cairo_bo_edge_t *edge;
    skip_elt_t elt;
};

#define SKIP_ELT_TO_EDGE_ELT(elt)	SKIP_LIST_ELT_TO_DATA (sweep_line_elt_t, (elt))
#define SKIP_ELT_TO_EDGE(elt) 		(SKIP_ELT_TO_EDGE_ELT (elt)->edge)

typedef enum {
    CAIRO_BO_STATUS_INTERSECTION,
    CAIRO_BO_STATUS_PARALLEL,
    CAIRO_BO_STATUS_NO_INTERSECTION
} cairo_bo_status_t;

typedef enum {
    CAIRO_BO_EVENT_TYPE_START,
    CAIRO_BO_EVENT_TYPE_STOP,
    CAIRO_BO_EVENT_TYPE_INTERSECTION
} cairo_bo_event_type_t;

typedef struct _cairo_bo_event {
    cairo_bo_event_type_t type;
    cairo_bo_edge_t *e1;
    cairo_bo_edge_t *e2;
    cairo_bo_point32_t point;
    skip_elt_t elt;
} cairo_bo_event_t;

#define SKIP_ELT_TO_EVENT(elt) SKIP_LIST_ELT_TO_DATA (cairo_bo_event_t, (elt))

typedef struct _cairo_bo_event_queue {
    skip_list_t intersection_queue;

    cairo_bo_event_t *startstop_events;
    cairo_bo_event_t **sorted_startstop_event_ptrs;
    unsigned next_startstop_event_index;
    unsigned num_startstop_events;
} cairo_bo_event_queue_t;


typedef struct _cairo_bo_sweep_line {
    skip_list_t active_edges;
    cairo_bo_edge_t *head;
    cairo_bo_edge_t *tail;
    int32_t current_y;
} cairo_bo_sweep_line_t;

static inline int
_cairo_bo_point32_compare (cairo_bo_point32_t const *a,
			   cairo_bo_point32_t const *b)
{
    int cmp = a->y - b->y;
    if (cmp) return cmp;
    return a->x - b->x;
}



































static int
_slope_compare (cairo_bo_edge_t *a,
		cairo_bo_edge_t *b)
{
    




    int32_t adx = a->bottom.x - a->top.x;
    int32_t bdx = b->bottom.x - b->top.x;

    


    if ((adx ^ bdx) < 0) {
	return adx < 0 ? -1 : +1;
    }
    else {
	int32_t ady = a->bottom.y - a->top.y;
	int32_t bdy = b->bottom.y - b->top.y;
	int64_t adx_bdy = _cairo_int32x32_64_mul (adx, bdy);
	int64_t bdx_ady = _cairo_int32x32_64_mul (bdx, ady);

	
	if (_cairo_int64_gt (adx_bdy, bdx_ady))
	    return 1;

	
	if (_cairo_int64_lt (adx_bdy, bdx_ady))
	    return -1;
	return 0;
    }
}

static cairo_quorem64_t
edge_x_for_y (cairo_bo_edge_t *edge,
	      int32_t y)
{
    




    int32_t dx = edge->bottom.x - edge->top.x;
    int32_t dy = edge->bottom.y - edge->top.y;
    int64_t numerator;
    cairo_quorem64_t quorem;

    if (edge->middle.y == y) {
       quorem.quo = edge->middle.x;
       quorem.rem = 0;
       return quorem;
    }
    if (edge->bottom.y == y) {
       quorem.quo = edge->bottom.x;
       quorem.rem = 0;
       return quorem;
    }
    if (dy == 0) {
	quorem.quo = _cairo_int32_to_int64 (edge->top.x);
	quorem.rem = 0;
	return quorem;
    }

    
    numerator = _cairo_int32x32_64_mul ((y - edge->top.y), dx);
    quorem = _cairo_int64_divrem (numerator, dy);
    quorem.quo += edge->top.x;

    return quorem;
}

static int
_cairo_bo_sweep_line_compare_edges (cairo_bo_sweep_line_t	*sweep_line,
				    cairo_bo_edge_t		*a,
				    cairo_bo_edge_t		*b)
{
    cairo_quorem64_t ax;
    cairo_quorem64_t bx;
    int cmp;

    if (a == b)
	return 0;

    

    {
           int32_t amin, amax;
           int32_t bmin, bmax;
           if (a->middle.x < a->bottom.x) {
                   amin = a->middle.x;
                   amax = a->bottom.x;
           } else {
                   amin = a->bottom.x;
                   amax = a->middle.x;
           }
           if (b->middle.x < b->bottom.x) {
                   bmin = b->middle.x;
                   bmax = b->bottom.x;
           } else {
                   bmin = b->bottom.x;
                   bmax = b->middle.x;
           }
           if (amax < bmin) return -1;
           if (amin > bmax) return +1;
    }

    ax = edge_x_for_y (a, sweep_line->current_y);
    bx = edge_x_for_y (b, sweep_line->current_y);
    if (ax.quo > bx.quo)
	return 1;
    else if (ax.quo < bx.quo)
	return -1;

    
    if (ax.rem > bx.rem)
	return 1;
    else if (ax.rem < bx.rem)
	return -1;

    




    cmp = _slope_compare (a, b);
    if (cmp)
	return cmp;

    

    

    cmp = _cairo_bo_point32_compare (&a->top, &b->top);
    if (cmp)
	return cmp;

    cmp = _cairo_bo_point32_compare (&a->bottom, &b->bottom);
    if (cmp)
	return cmp;

    



    if (a > b)
	return 1;
    else
	return -1;
}

static int
_sweep_line_elt_compare (void	*list,
			 void	*a,
			 void	*b)
{
    cairo_bo_sweep_line_t *sweep_line = list;
    sweep_line_elt_t *edge_elt_a = a;
    sweep_line_elt_t *edge_elt_b = b;

    return _cairo_bo_sweep_line_compare_edges (sweep_line,
					       edge_elt_a->edge,
					       edge_elt_b->edge);
}

static inline int
cairo_bo_event_compare (cairo_bo_event_t const *a,
			cairo_bo_event_t const *b)
{
    int cmp;

    





    cmp = _cairo_bo_point32_compare (&a->point, &b->point);
    if (cmp)
	return cmp;

    




    if (a->type != b->type) {
	if (a->type == CAIRO_BO_EVENT_TYPE_STOP)
	    return -1;
	if (a->type == CAIRO_BO_EVENT_TYPE_START)
	    return 1;

	if (b->type == CAIRO_BO_EVENT_TYPE_STOP)
	    return 1;
	if (b->type == CAIRO_BO_EVENT_TYPE_START)
	    return -1;
    }

    









    cmp = _slope_compare (a->e1, b->e1);
    if (cmp) {
	if (a->type == CAIRO_BO_EVENT_TYPE_START)
	    return cmp;
	else
	    return - cmp;
    }

    

    if (a->type == CAIRO_BO_EVENT_TYPE_START) {
	cmp = _cairo_bo_point32_compare (&b->e1->bottom,
					 &a->e1->bottom);
	if (cmp)
	    return cmp;
    }
    else if (a->type == CAIRO_BO_EVENT_TYPE_STOP) {
	cmp = _cairo_bo_point32_compare (&a->e1->top,
					 &b->e1->top);
	if (cmp)
	    return cmp;
    }
    else { 
	



	cmp = _cairo_bo_point32_compare (&a->e2->top, &b->e2->top);
	if (cmp)
	    return cmp;
	cmp = _cairo_bo_point32_compare (&a->e2->bottom, &b->e2->bottom);
	if (cmp)
	    return cmp;
	cmp = _cairo_bo_point32_compare (&a->e1->top, &b->e1->top);
	if (cmp)
	    return cmp;
	cmp = _cairo_bo_point32_compare (&a->e1->bottom, &b->e1->bottom);
	if (cmp)
	    return cmp;
     }

    
    if (a->e1 < b->e1)
	return -1;
    if (a->e1 > b->e1)
	return +1;
    if (a->e2 < b->e2)
	return -1;
    if (a->e2 > b->e2)
	return +1;
    return 0;
}

static int
cairo_bo_event_compare_abstract (void		*list,
				 void	*a,
				 void	*b)
{
    cairo_bo_event_t *event_a = a;
    cairo_bo_event_t *event_b = b;

    return cairo_bo_event_compare (event_a, event_b);
}

static int
cairo_bo_event_compare_pointers (void const *voida,
				 void const *voidb)
{
    cairo_bo_event_t const * const *a = voida;
    cairo_bo_event_t const * const *b = voidb;
    if (*a != *b) {
	int cmp = cairo_bo_event_compare (*a, *b);
	if (cmp)
	    return cmp;
	if (*a < *b)
	    return -1;
	if (*a > *b)
	    return +1;
    }
    return 0;
}

static inline cairo_int64_t
det32_64 (int32_t a,
	  int32_t b,
	  int32_t c,
	  int32_t d)
{
    cairo_int64_t ad;
    cairo_int64_t bc;

    
    ad = _cairo_int32x32_64_mul (a, d);
    bc = _cairo_int32x32_64_mul (b, c);

    return _cairo_int64_sub (ad, bc);
}

static inline cairo_int128_t
det64_128 (cairo_int64_t a,
	   cairo_int64_t b,
	   cairo_int64_t c,
	   cairo_int64_t d)
{
    cairo_int128_t ad;
    cairo_int128_t bc;

    
    ad = _cairo_int64x64_128_mul (a, d);
    bc = _cairo_int64x64_128_mul (b, c);

    return _cairo_int128_sub (ad, bc);
}







static cairo_bo_status_t
intersect_lines (cairo_bo_edge_t		*a,
		 cairo_bo_edge_t		*b,
		 cairo_bo_intersect_point_t	*intersection)
{
    cairo_int64_t a_det, b_det;

    





    int32_t dx1 = a->top.x - a->bottom.x;
    int32_t dy1 = a->top.y - a->bottom.y;

    int32_t dx2 = b->top.x - b->bottom.x;
    int32_t dy2 = b->top.y - b->bottom.y;

    cairo_int64_t den_det = det32_64 (dx1, dy1, dx2, dy2);
    cairo_quorem64_t qr;

    if (_cairo_int64_eq (den_det, 0))
	return CAIRO_BO_STATUS_PARALLEL;

    a_det = det32_64 (a->top.x, a->top.y,
		      a->bottom.x, a->bottom.y);
    b_det = det32_64 (b->top.x, b->top.y,
		      b->bottom.x, b->bottom.y);

    
    qr = _cairo_int_96by64_32x64_divrem (det64_128 (a_det, dx1,
						    b_det, dx2),
					 den_det);
    if (_cairo_int64_eq (qr.rem,den_det))
	return CAIRO_BO_STATUS_NO_INTERSECTION;
    intersection->x.ordinate = qr.quo;
    intersection->x.exactness = qr.rem ? INEXACT : EXACT;

    
    qr = _cairo_int_96by64_32x64_divrem (det64_128 (a_det, dy1,
						    b_det, dy2),
					 den_det);
    if (_cairo_int64_eq (qr.rem, den_det))
	return CAIRO_BO_STATUS_NO_INTERSECTION;
    intersection->y.ordinate = qr.quo;
    intersection->y.exactness = qr.rem ? INEXACT : EXACT;

    return CAIRO_BO_STATUS_INTERSECTION;
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

    






    cmp_top = _cairo_bo_intersect_ordinate_32_compare (point->y, edge->top.y);
    cmp_bottom = _cairo_bo_intersect_ordinate_32_compare (point->y, edge->bottom.y);

    if (cmp_top < 0 || cmp_bottom > 0)
    {
	return FALSE;
    }

    if (cmp_top > 0 && cmp_bottom < 0)
    {
	return TRUE;
    }

    



    






    if (cmp_top == 0)
	return (_cairo_bo_intersect_ordinate_32_compare (point->x, edge->top.x) > 0);
    else 
	return (_cairo_bo_intersect_ordinate_32_compare (point->x, edge->bottom.x) < 0);
}

















static cairo_bo_status_t
_cairo_bo_edge_intersect (cairo_bo_edge_t	*a,
			  cairo_bo_edge_t	*b,
			  cairo_bo_point32_t	*intersection)
{
    cairo_bo_status_t status;
    cairo_bo_intersect_point_t quorem;

    status = intersect_lines (a, b, &quorem);
    if (status)
	return status;

    if (! _cairo_bo_edge_contains_intersect_point (a, &quorem))
	return CAIRO_BO_STATUS_NO_INTERSECTION;

    if (! _cairo_bo_edge_contains_intersect_point (b, &quorem))
	return CAIRO_BO_STATUS_NO_INTERSECTION;

    




    intersection->x = quorem.x.ordinate;
    intersection->y = quorem.y.ordinate;

    return CAIRO_BO_STATUS_INTERSECTION;
}

static void
_cairo_bo_event_init (cairo_bo_event_t		*event,
		      cairo_bo_event_type_t	 type,
		      cairo_bo_edge_t	*e1,
		      cairo_bo_edge_t	*e2,
		      cairo_bo_point32_t	 point)
{
    event->type = type;
    event->e1 = e1;
    event->e2 = e2;
    event->point = point;
}

static void
_cairo_bo_event_queue_insert (cairo_bo_event_queue_t *queue,
			      cairo_bo_event_t	     *event)
{
    
    skip_list_insert (&queue->intersection_queue, event,
		      event->type == CAIRO_BO_EVENT_TYPE_INTERSECTION);
}

static void
_cairo_bo_event_queue_delete (cairo_bo_event_queue_t *queue,
			      cairo_bo_event_t	     *event)
{
    if (CAIRO_BO_EVENT_TYPE_INTERSECTION == event->type)
	skip_list_delete_given ( &queue->intersection_queue, &event->elt );
}

static cairo_bo_event_t *
_cairo_bo_event_dequeue (cairo_bo_event_queue_t *event_queue)
{
    skip_elt_t *elt = event_queue->intersection_queue.chains[0];
    cairo_bo_event_t *intersection = elt ? SKIP_ELT_TO_EVENT (elt) : NULL;
    cairo_bo_event_t *startstop;

    if (event_queue->next_startstop_event_index == event_queue->num_startstop_events)
	return intersection;
    startstop = event_queue->sorted_startstop_event_ptrs[
	event_queue->next_startstop_event_index];

    if (!intersection || cairo_bo_event_compare (startstop, intersection) <= 0)
    {
	event_queue->next_startstop_event_index++;
	return startstop;
    }
    return intersection;
}

static cairo_status_t
_cairo_bo_event_queue_init (cairo_bo_event_queue_t	*event_queue,
			    cairo_bo_edge_t	*edges,
			    int				 num_edges)
{
    int i;
    cairo_bo_event_t *events, **sorted_event_ptrs;
    unsigned num_events = 2*num_edges;

    memset (event_queue, 0, sizeof(*event_queue));

    skip_list_init (&event_queue->intersection_queue,
		    cairo_bo_event_compare_abstract,
		    sizeof (cairo_bo_event_t));
    if (0 == num_edges)
	return CAIRO_STATUS_SUCCESS;

    



    events = malloc (num_events * sizeof(cairo_bo_event_t));
    sorted_event_ptrs = malloc (num_events * sizeof(cairo_bo_event_t*));
    if (!events || !sorted_event_ptrs) {
	if (events) free(events);
	if (sorted_event_ptrs) free(sorted_event_ptrs);
	return CAIRO_STATUS_NO_MEMORY;
    }
    event_queue->startstop_events = events;
    event_queue->sorted_startstop_event_ptrs = sorted_event_ptrs;
    event_queue->num_startstop_events = (unsigned)(num_events);
    event_queue->next_startstop_event_index = 0;

    for (i = 0; i < num_edges; i++) {
	sorted_event_ptrs[2*i] = &events[2*i];
	sorted_event_ptrs[2*i+1] = &events[2*i+1];

	
	edges[i].middle = edges[i].top;

	_cairo_bo_event_init (&events[2*i],
			      CAIRO_BO_EVENT_TYPE_START,
			      &edges[i], NULL,
			      edges[i].top);

	_cairo_bo_event_init (&events[2*i+1],
			      CAIRO_BO_EVENT_TYPE_STOP,
			      &edges[i], NULL,
			      edges[i].bottom);
    }

    qsort (sorted_event_ptrs, num_events,
	   sizeof(cairo_bo_event_t *),
	   cairo_bo_event_compare_pointers);
    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_bo_event_queue_fini (cairo_bo_event_queue_t *event_queue)
{
    skip_list_fini (&event_queue->intersection_queue);
    if (event_queue->startstop_events)
	free (event_queue->startstop_events);
    if (event_queue->sorted_startstop_event_ptrs)
	free (event_queue->sorted_startstop_event_ptrs);
}

static void
_cairo_bo_event_queue_insert_if_intersect_below_current_y (cairo_bo_event_queue_t	*event_queue,
							   cairo_bo_edge_t	*left,
							   cairo_bo_edge_t	*right)
{
    cairo_bo_status_t status;
    cairo_bo_point32_t intersection;
    cairo_bo_event_t event;

    if (left == NULL || right == NULL)
	return;

    




    if (_slope_compare (left, right) < 0)
	return;

    status = _cairo_bo_edge_intersect (left, right, &intersection);
    if (status == CAIRO_BO_STATUS_PARALLEL ||
	status == CAIRO_BO_STATUS_NO_INTERSECTION)
    {
	return;
    }

    _cairo_bo_event_init (&event,
			  CAIRO_BO_EVENT_TYPE_INTERSECTION,
			  left, right,
			  intersection);

    _cairo_bo_event_queue_insert (event_queue, &event);
}

static void
_cairo_bo_sweep_line_init (cairo_bo_sweep_line_t *sweep_line)
{
    skip_list_init (&sweep_line->active_edges,
		    _sweep_line_elt_compare,
		    sizeof (sweep_line_elt_t));
    sweep_line->head = NULL;
    sweep_line->tail = NULL;
    sweep_line->current_y = 0;
}

static void
_cairo_bo_sweep_line_fini (cairo_bo_sweep_line_t *sweep_line)
{
    skip_list_fini (&sweep_line->active_edges);
}

static void
_cairo_bo_sweep_line_insert (cairo_bo_sweep_line_t	*sweep_line,
			     cairo_bo_edge_t		*edge)
{
    skip_elt_t *next_elt;
    sweep_line_elt_t *sweep_line_elt;
    cairo_bo_edge_t **prev_of_next, **next_of_prev;

    sweep_line_elt = skip_list_insert (&sweep_line->active_edges, &edge,
				       1 );

    next_elt = sweep_line_elt->elt.next[0];
    if (next_elt)
	prev_of_next = & (SKIP_ELT_TO_EDGE (next_elt)->prev);
    else
	prev_of_next = &sweep_line->tail;

    if (*prev_of_next)
	next_of_prev = &(*prev_of_next)->next;
    else
	next_of_prev = &sweep_line->head;

    edge->prev = *prev_of_next;
    edge->next = *next_of_prev;
    *prev_of_next = edge;
    *next_of_prev = edge;

    edge->sweep_line_elt = sweep_line_elt;
}

static void
_cairo_bo_sweep_line_delete (cairo_bo_sweep_line_t	*sweep_line,
			     cairo_bo_edge_t	*edge)
{
    cairo_bo_edge_t **left_next, **right_prev;

    skip_list_delete_given (&sweep_line->active_edges, &edge->sweep_line_elt->elt);

    left_next = &sweep_line->head;
    if (edge->prev)
	left_next = &edge->prev->next;

    right_prev = &sweep_line->tail;
    if (edge->next)
	right_prev = &edge->next->prev;

    *left_next = edge->next;
    *right_prev = edge->prev;
}

static void
_cairo_bo_sweep_line_swap (cairo_bo_sweep_line_t	*sweep_line,
			   cairo_bo_edge_t		*left,
			   cairo_bo_edge_t		*right)
{
    sweep_line_elt_t *left_elt, *right_elt;
    cairo_bo_edge_t **before_left, **after_right;

    


    left_elt = left->sweep_line_elt;
    right_elt = SKIP_ELT_TO_EDGE_ELT (left_elt->elt.next[0]);

    left_elt->edge = right;
    right->sweep_line_elt = left_elt;

    right_elt->edge = left;
    left->sweep_line_elt = right_elt;

    

    before_left = &sweep_line->head;
    if (left->prev)
	before_left = &left->prev->next;
    *before_left = right;

    after_right = &sweep_line->tail;
    if (right->next)
	after_right = &right->next->prev;
    *after_right = left;

    left->next = right->next;
    right->next = left;

    right->prev = left->prev;
    left->prev = right;
}

#define DEBUG_PRINT_STATE 0
#if DEBUG_PRINT_STATE
static void
_cairo_bo_edge_print (cairo_bo_edge_t *edge)
{
    printf ("(0x%x, 0x%x)-(0x%x, 0x%x)",
	    edge->top.x, edge->top.y,
	    edge->bottom.x, edge->bottom.y);
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
    skip_elt_t *elt;
    
    skip_list_t *queue = &event_queue->intersection_queue;
    cairo_bo_event_t *event;

    printf ("Event queue:\n");

    for (elt = queue->chains[0];
	 elt;
	 elt = elt->next[0])
    {
	event = SKIP_ELT_TO_EVENT (elt);
	_cairo_bo_event_print (event);
    }
}

static void
_cairo_bo_sweep_line_print (cairo_bo_sweep_line_t *sweep_line)
{
    cairo_bool_t first = TRUE;
    skip_elt_t *elt;
    cairo_bo_edge_t *edge;

    printf ("Sweep line (reversed):     ");

    for (edge = sweep_line->tail;
	 edge;
	 edge = edge->prev)
    {
	if (!first)
	    printf (", ");
	_cairo_bo_edge_print (edge);
	first = FALSE;
    }
    printf ("\n");


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

    printf ("Sweep line from skip list: ");
    first = TRUE;
    for (elt = sweep_line->active_edges.chains[0];
	 elt;
	 elt = elt->next[0])
    {
	if (!first)
	    printf (", ");
	_cairo_bo_edge_print (SKIP_ELT_TO_EDGE (elt));
	first = FALSE;
    }
    printf ("\n");
}

static void
print_state (const char			*msg,
	     cairo_bo_event_queue_t	*event_queue,
	     cairo_bo_sweep_line_t	*sweep_line)
{
    printf ("%s\n", msg);
    _cairo_bo_event_queue_print (event_queue);
    _cairo_bo_sweep_line_print (sweep_line);
    printf ("\n");
}
#endif



static cairo_status_t
_cairo_bo_edge_end_trap (cairo_bo_edge_t	*left,
			 int32_t		bot,
			 cairo_bo_traps_t	*bo_traps)
{
    cairo_fixed_t fixed_top, fixed_bot;
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    cairo_bo_trap_t *trap = left->deferred_trap;
    cairo_bo_edge_t *right;

    if (!trap)
	return CAIRO_STATUS_SUCCESS;

     

    right = trap->right;
    if (right->bottom.y < bot)
	bot = right->bottom.y;

    fixed_top = trap->top;
    fixed_bot = bot;

    
    if (fixed_top < fixed_bot) {
	cairo_point_t left_top, left_bot, right_top, right_bot;
	cairo_fixed_t xmin = bo_traps->xmin;
	cairo_fixed_t ymin = bo_traps->ymin;
	fixed_top += ymin;
	fixed_bot += ymin;

	left_top.x = left->top.x + xmin;
	left_top.y = left->top.y + ymin;
	right_top.x = right->top.x + xmin;
	right_top.y = right->top.y + ymin;
	left_bot.x = left->bottom.x + xmin;
	left_bot.y = left->bottom.y + ymin;
	right_bot.x = right->bottom.x + xmin;
	right_bot.y = right->bottom.y + ymin;

	



	if (left_top.x != right_top.x ||
	    left_top.y != right_top.y ||
	    left_bot.x != right_bot.x ||
	    left_bot.y != right_bot.y)
	{
	    status = _cairo_traps_add_trap_from_points (bo_traps->traps,
							fixed_top,
							fixed_bot,
							left_top, left_bot,
							right_top, right_bot);

#if DEBUG_PRINT_STATE
	    printf ("Deferred trap: left=(%08x, %08x)-(%08x,%08x) "
		    "right=(%08x,%08x)-(%08x,%08x) top=%08x, bot=%08x\n",
		    left->top.x, left->top.y, left->bottom.x, left->bottom.y,
		    right->top.x, right->top.y, right->bottom.x, right->bottom.y,
		    trap->top, bot);
#endif
	}
    }

    _cairo_freelist_free (&bo_traps->freelist, trap);
    left->deferred_trap = NULL;
    return status;
}






static cairo_status_t
_cairo_bo_edge_start_or_continue_trap (cairo_bo_edge_t	*edge,
				       int32_t		top,
				       cairo_bo_traps_t	*bo_traps)
{
    cairo_status_t status;
    cairo_bo_trap_t *trap = edge->deferred_trap;

    if (trap) {
	if (trap->right == edge->next) return CAIRO_STATUS_SUCCESS;
	status = _cairo_bo_edge_end_trap (edge, top, bo_traps);
	if (status)
	    return status;
    }

    if (edge->next) {
	trap = edge->deferred_trap = _cairo_freelist_alloc (&bo_traps->freelist);
	if (!edge->deferred_trap)
	    return CAIRO_STATUS_NO_MEMORY;

	trap->right = edge->next;
	trap->top = top;
    }
    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_bo_traps_init (cairo_bo_traps_t	*bo_traps,
		      cairo_traps_t	*traps,
		      cairo_fixed_t	 xmin,
		      cairo_fixed_t	 ymin,
		      cairo_fixed_t	 xmax,
		      cairo_fixed_t	 ymax)
{
    bo_traps->traps = traps;
    _cairo_freelist_init (&bo_traps->freelist, sizeof(cairo_bo_trap_t));
    bo_traps->xmin = xmin;
    bo_traps->ymin = ymin;
    bo_traps->xmax = xmax;
    bo_traps->ymax = ymax;
}

static void
_cairo_bo_traps_fini (cairo_bo_traps_t *bo_traps)
{
    _cairo_freelist_fini (&bo_traps->freelist);
}

static void
_cairo_bo_sweep_line_validate (cairo_bo_sweep_line_t *sweep_line)
{
    cairo_bo_edge_t *edge;
    skip_elt_t *elt;

    



    for (edge = sweep_line->head, elt = sweep_line->active_edges.chains[0];
	 edge && elt;
	 edge = edge->next, elt = elt->next[0])
    {
	if (SKIP_ELT_TO_EDGE (elt) != edge) {
	    fprintf (stderr, "*** Error: Sweep line fails to validate: Inconsistent data in the two lists.\n");
	    exit (1);
	}
    }

    if (edge || elt) {
	fprintf (stderr, "*** Error: Sweep line fails to validate: One list ran out before the other.\n");
	exit (1);
    }
}


static cairo_status_t
_active_edges_to_traps (cairo_bo_edge_t		*head,
			int32_t			 top,
			cairo_fill_rule_t	 fill_rule,
			cairo_bo_traps_t	*bo_traps)
{
    cairo_status_t status;
    int in_out = 0;
    cairo_bo_edge_t *edge;

    for (edge = head; edge && edge->next; edge = edge->next) {
	if (fill_rule == CAIRO_FILL_RULE_WINDING) {
	    if (edge->reversed)
		in_out++;
	    else
		in_out--;
	    if (in_out == 0) {
		status = _cairo_bo_edge_end_trap (edge, top, bo_traps);
		if (status)
		    return status;
		continue;
	    }
	} else {
	    in_out++;
	    if ((in_out & 1) == 0) {
		status = _cairo_bo_edge_end_trap (edge, top, bo_traps);
		if (status)
		    return status;
		continue;
	    }
	}

	status = _cairo_bo_edge_start_or_continue_trap (edge, top, bo_traps);
	if (status)
	    return status;
    }

    return CAIRO_STATUS_SUCCESS;
}




static cairo_status_t
_cairo_bentley_ottmann_tessellate_bo_edges (cairo_bo_edge_t	*edges,
					    int			 num_edges,
					    cairo_fill_rule_t	 fill_rule,
					    cairo_traps_t	*traps,
					    cairo_fixed_t	xmin,
					    cairo_fixed_t	ymin,
					    cairo_fixed_t	xmax,
					    cairo_fixed_t	ymax,
					    int			*num_intersections)
{
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    int intersection_count = 0;
    cairo_bo_event_queue_t event_queue;
    cairo_bo_sweep_line_t sweep_line;
    cairo_bo_traps_t bo_traps;
    cairo_bo_event_t *event, event_saved;
    cairo_bo_edge_t *edge;
    cairo_bo_edge_t *left, *right;
    cairo_bo_edge_t *edge1, *edge2;

    _cairo_bo_event_queue_init (&event_queue, edges, num_edges);
    _cairo_bo_sweep_line_init (&sweep_line);
    _cairo_bo_traps_init (&bo_traps, traps, xmin, ymin, xmax, ymax);

#if DEBUG_PRINT_STATE
    print_state ("After initializing", &event_queue, &sweep_line);
#endif

    while (1)
    {
	event = _cairo_bo_event_dequeue (&event_queue);
	if (!event)
	    break;

	if (event->point.y != sweep_line.current_y) {
	    status = _active_edges_to_traps (sweep_line.head,
					     sweep_line.current_y,
					     fill_rule, &bo_traps);
	    if (status)
		goto unwind;

	    sweep_line.current_y = event->point.y;
	}

	event_saved = *event;
	_cairo_bo_event_queue_delete (&event_queue, event);
	event = &event_saved;

	switch (event->type) {
	case CAIRO_BO_EVENT_TYPE_START:
	    edge = event->e1;

	    _cairo_bo_sweep_line_insert (&sweep_line, edge);
	    



	    left = edge->prev;
	    right = edge->next;

	    _cairo_bo_event_queue_insert_if_intersect_below_current_y (&event_queue, left, edge);

	    _cairo_bo_event_queue_insert_if_intersect_below_current_y (&event_queue, edge, right);

#if DEBUG_PRINT_STATE
	    print_state ("After processing start", &event_queue, &sweep_line);
#endif
	    _cairo_bo_sweep_line_validate (&sweep_line);

	    break;
	case CAIRO_BO_EVENT_TYPE_STOP:
	    edge = event->e1;

	    left = edge->prev;
	    right = edge->next;

	    _cairo_bo_sweep_line_delete (&sweep_line, edge);

	    status = _cairo_bo_edge_end_trap (edge, edge->bottom.y, &bo_traps);
	    if (status)
		goto unwind;

	    _cairo_bo_event_queue_insert_if_intersect_below_current_y (&event_queue, left, right);

#if DEBUG_PRINT_STATE
	    print_state ("After processing stop", &event_queue, &sweep_line);
#endif
	    _cairo_bo_sweep_line_validate (&sweep_line);

	    break;
	case CAIRO_BO_EVENT_TYPE_INTERSECTION:
	    edge1 = event->e1;
	    edge2 = event->e2;

	    
	    if (edge2 != edge1->next)
		break;

	    intersection_count++;

	    edge1->middle = event->point;
	    edge2->middle = event->point;

	    left = edge1->prev;
	    right = edge2->next;

	    _cairo_bo_sweep_line_swap (&sweep_line, edge1, edge2);

	    

	    _cairo_bo_event_queue_insert_if_intersect_below_current_y (&event_queue,
								       left, edge2);

	    _cairo_bo_event_queue_insert_if_intersect_below_current_y (&event_queue,
								       edge1, right);

#if DEBUG_PRINT_STATE
	    print_state ("After processing intersection", &event_queue, &sweep_line);
#endif
	    _cairo_bo_sweep_line_validate (&sweep_line);

	    break;
	}
    }

    *num_intersections = intersection_count;
 unwind:
    for (edge = sweep_line.head; edge; edge = edge->next) {
	cairo_status_t status2 = _cairo_bo_edge_end_trap (edge,
							  sweep_line.current_y,
							  &bo_traps);
	if (!status)
	    status = status2;
    }
    _cairo_bo_traps_fini (&bo_traps);
    _cairo_bo_sweep_line_fini (&sweep_line);
    _cairo_bo_event_queue_fini (&event_queue);
    return status;
}

static void
update_minmax(cairo_fixed_t *inout_min,
	      cairo_fixed_t *inout_max,
	      cairo_fixed_t v)
{
    if (v < *inout_min)
	*inout_min = v;
    if (v > *inout_max)
	*inout_max = v;
}

cairo_status_t
_cairo_bentley_ottmann_tessellate_polygon (cairo_traps_t	*traps,
					   cairo_polygon_t	*polygon,
					   cairo_fill_rule_t	 fill_rule)
{
    int intersections;
    cairo_status_t status;
    cairo_bo_edge_t *edges;
    cairo_fixed_t xmin = 0x7FFFFFFF;
    cairo_fixed_t ymin = 0x7FFFFFFF;
    cairo_fixed_t xmax = -0x80000000;
    cairo_fixed_t ymax = -0x80000000;
    int num_bo_edges;
    int i;

    if (0 == polygon->num_edges)
	return CAIRO_STATUS_SUCCESS;

    edges = malloc (polygon->num_edges * sizeof (cairo_bo_edge_t));
    if (edges == NULL)
	return CAIRO_STATUS_NO_MEMORY;

    

    for (i = 0; i < polygon->num_edges; i++) {
	update_minmax (&xmin, &xmax, polygon->edges[i].edge.p1.x);
	update_minmax (&ymin, &ymax, polygon->edges[i].edge.p1.y);
	update_minmax (&xmin, &xmax, polygon->edges[i].edge.p2.x);
	update_minmax (&ymin, &ymax, polygon->edges[i].edge.p2.y);
	assert (polygon->edges[i].edge.p1.y <= polygon->edges[i].edge.p2.y &&
		"BUG: tessellator given upside down or horizontal edges");
    }

    





    


    if (xmax - xmin < 0)
	xmax = xmin + 0x7FFFFFFF;
    if (ymax - ymin < 0)
	ymax = ymin + 0x7FFFFFFF;

    for (i = 0, num_bo_edges = 0; i < polygon->num_edges; i++) {
	cairo_bo_edge_t *edge = &edges[num_bo_edges];
	cairo_point_t top = polygon->edges[i].edge.p1;
	cairo_point_t bot = polygon->edges[i].edge.p2;

	
	top.x -= xmin;
	top.y -= ymin;
	bot.x -= xmin;
	bot.y -= ymin;

	


	if (top.x < 0) top.x = xmax - xmin;
	if (top.y < 0) top.y = ymax - ymin;
	if (bot.x < 0) bot.x = xmax - xmin;
	if (bot.y < 0) bot.y = ymax - ymin;

	if (top.y == bot.y) {
	    

	    continue;
	}
	assert (top.y < bot.y &&
		"BUG: clamping the input range flipped the "
		"orientation of an edge");

	edge->top.x = top.x;
	edge->top.y = top.y;
	edge->bottom.x = bot.x;
	edge->bottom.y = bot.y;
	


	edge->reversed = (! polygon->edges[i].clockWise);
	edge->deferred_trap = NULL;
	edge->prev = NULL;
	edge->next = NULL;
	edge->sweep_line_elt = NULL;

	num_bo_edges++;
    }

    



    status = _cairo_bentley_ottmann_tessellate_bo_edges (edges, num_bo_edges,
							 fill_rule, traps,
							 xmin, ymin, xmax, ymax,
							 &intersections);

    free (edges);

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
    cairo_bo_status_t status;

    
    for (i = 0; i < num_edges; i++) {
	assert (_cairo_bo_point32_compare (&edges[i].top, &edges[i].bottom) < 0);
	edges[i].top.x <<= CAIRO_BO_GUARD_BITS;
	edges[i].top.y <<= CAIRO_BO_GUARD_BITS;
	edges[i].bottom.x <<= CAIRO_BO_GUARD_BITS;
	edges[i].bottom.y <<= CAIRO_BO_GUARD_BITS;
    }

    for (i = 0; i < num_edges; i++) {
	for (j = 0; j < num_edges; j++) {
	    if (i == j)
		continue;

	    a = &edges[i];
	    b = &edges[j];

	    status = _cairo_bo_edge_intersect (a, b, &intersection);
	    if (status == CAIRO_BO_STATUS_PARALLEL ||
		status == CAIRO_BO_STATUS_NO_INTERSECTION)
	    {
		continue;
	    }

	    printf ("Found intersection (%d,%d) between (%d,%d)-(%d,%d) and (%d,%d)-(%d,%d)\n",
		    intersection.x,
		    intersection.y,
		    a->top.x, a->top.y,
		    a->bottom.x, a->bottom.y,
		    b->top.x, b->top.y,
		    b->bottom.x, b->bottom.y);

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
	edges = malloc (num_edges * sizeof (cairo_bo_edge_t));
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
    static cairo_bo_edge_t random_edges[MAX_RANDOM], *edge;
    unsigned int i, num_random;
    test_t *test;

    for (i = 0; i < sizeof (tests) / sizeof (tests[0]); i++) {
	test = &tests[i];
	run_test (test->name, test->edges, test->num_edges);
    }

    for (num_random = 0; num_random < MAX_RANDOM; num_random++) {
	srand (0);
	for (i = 0; i < num_random; i++) {
	    do {
		edge = &random_edges[i];
		edge->top.x = (int32_t) (10.0 * (rand() / (RAND_MAX + 1.0)));
		edge->top.y = (int32_t) (10.0 * (rand() / (RAND_MAX + 1.0)));
		edge->bottom.x = (int32_t) (10.0 * (rand() / (RAND_MAX + 1.0)));
		edge->bottom.y = (int32_t) (10.0 * (rand() / (RAND_MAX + 1.0)));
		if (edge->top.y > edge->bottom.y) {
		    int32_t tmp = edge->top.y;
		    edge->top.y = edge->bottom.y;
		    edge->bottom.y = tmp;
		}
	    } while (edge->top.y == edge->bottom.y);
	}

	sprintf (random_name, "random-%02d", num_random);

	run_test (random_name, random_edges, num_random);
    }

    return 0;
}
#endif

