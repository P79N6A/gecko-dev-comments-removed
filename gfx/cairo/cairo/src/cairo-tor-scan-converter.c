






























































































#include "cairoint.h"
#include "cairo-spans-private.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>




#define I static


#define GLITTER_HAVE_STATUS_T 1
#define GLITTER_STATUS_SUCCESS CAIRO_STATUS_SUCCESS
#define GLITTER_STATUS_NO_MEMORY CAIRO_STATUS_NO_MEMORY
typedef cairo_status_t glitter_status_t;


#define GLITTER_INPUT_BITS CAIRO_FIXED_FRAC_BITS
#define GRID_X_BITS CAIRO_FIXED_FRAC_BITS
#define GRID_Y 15



struct pool;
struct cell_list;

static glitter_status_t
blit_with_span_renderer(
    struct cell_list		*coverages,
    cairo_span_renderer_t	*span_renderer,
    struct pool			*span_pool,
    int				 y,
    int				 xmin,
    int				 xmax);

#define GLITTER_BLIT_COVERAGES_ARGS \
	cairo_span_renderer_t *span_renderer, \
	struct pool *span_pool

#define GLITTER_BLIT_COVERAGES(cells, y, xmin, xmax) do {		\
    cairo_status_t status = blit_with_span_renderer (cells,		\
						     span_renderer,	\
						     span_pool,		\
						     y, xmin, xmax);	\
    if (unlikely (status))						\
	return status;							\
} while (0)











#ifndef GLITTER_INPUT_BITS
#  define GLITTER_INPUT_BITS 8
#endif
#define GLITTER_INPUT_SCALE (1<<GLITTER_INPUT_BITS)
typedef int glitter_input_scaled_t;

#if !GLITTER_HAVE_STATUS_T
typedef enum {
    GLITTER_STATUS_SUCCESS = 0,
    GLITTER_STATUS_NO_MEMORY
} glitter_status_t;
#endif

#ifndef I
# define I
#endif


typedef struct glitter_scan_converter glitter_scan_converter_t;





I glitter_status_t
glitter_scan_converter_reset(
    glitter_scan_converter_t *converter,
    int xmin, int ymin,
    int xmax, int ymax);






I glitter_status_t
glitter_scan_converter_add_edge(
    glitter_scan_converter_t *converter,
    glitter_input_scaled_t x1, glitter_input_scaled_t y1,
    glitter_input_scaled_t x2, glitter_input_scaled_t y2,
    int dir);











#ifndef GLITTER_BLIT_COVERAGES_ARGS
# define GLITTER_BLIT_COVERAGES_ARGS unsigned char *raster_pixels, long raster_stride
#endif
I glitter_status_t
glitter_scan_converter_render(
    glitter_scan_converter_t *converter,
    int nonzero_fill,
    GLITTER_BLIT_COVERAGES_ARGS);




#include <stdlib.h>
#include <string.h>
#include <limits.h>




typedef int grid_scaled_t;
typedef int grid_scaled_x_t;
typedef int grid_scaled_y_t;




#if !defined(GRID_X) && !defined(GRID_X_BITS)
#  define GRID_X_BITS 8
#endif
#if !defined(GRID_Y) && !defined(GRID_Y_BITS)
#  define GRID_Y 15
#endif


#ifdef GRID_X_BITS
#  define GRID_X (1 << GRID_X_BITS)
#endif
#ifdef GRID_Y_BITS
#  define GRID_Y (1 << GRID_Y_BITS)
#endif



#if defined(GRID_X_TO_INT_FRAC)
  
#elif defined(GRID_X_BITS)
#  define GRID_X_TO_INT_FRAC(x, i, f) \
	_GRID_TO_INT_FRAC_shift(x, i, f, GRID_X_BITS)
#else
#  define GRID_X_TO_INT_FRAC(x, i, f) \
	_GRID_TO_INT_FRAC_general(x, i, f, GRID_X)
#endif

#define _GRID_TO_INT_FRAC_general(t, i, f, m) do {	\
    (i) = (t) / (m);					\
    (f) = (t) % (m);					\
    if ((f) < 0) {					\
	--(i);						\
	(f) += (m);					\
    }							\
} while (0)

#define _GRID_TO_INT_FRAC_shift(t, i, f, b) do {	\
    (f) = (t) & ((1 << (b)) - 1);			\
    (i) = (t) >> (b);					\
} while (0)






typedef int grid_area_t;
#define GRID_XY (2*GRID_X*GRID_Y) /* Unit area on the grid. */


#if GRID_XY == 510
#  define GRID_AREA_TO_ALPHA(c)	  (((c)+1) >> 1)
#elif GRID_XY == 255
#  define  GRID_AREA_TO_ALPHA(c)  (c)
#elif GRID_XY == 64
#  define  GRID_AREA_TO_ALPHA(c)  (((c) << 2) | -(((c) & 0x40) >> 6))
#elif GRID_XY == 128
#  define  GRID_AREA_TO_ALPHA(c)  ((((c) << 1) | -((c) >> 7)) & 255)
#elif GRID_XY == 256
#  define  GRID_AREA_TO_ALPHA(c)  (((c) | -((c) >> 8)) & 255)
#elif GRID_XY == 15
#  define  GRID_AREA_TO_ALPHA(c)  (((c) << 4) + (c))
#elif GRID_XY == 2*256*15
#  define  GRID_AREA_TO_ALPHA(c)  (((c) + ((c)<<4) + 256) >> 9)
#else
#  define  GRID_AREA_TO_ALPHA(c)  (((c)*255 + GRID_XY/2) / GRID_XY)
#endif

#define UNROLL3(x) x x x

struct quorem {
    int quo;
    int rem;
};


struct _pool_chunk {
    
    size_t size;

    
    size_t capacity;

    

    struct _pool_chunk *prev_chunk;

    
};





struct pool {
    
    struct _pool_chunk *current;

    

    struct _pool_chunk *first_free;

    
    size_t default_capacity;

    


    struct _pool_chunk sentinel[1];
};


struct edge {
    
    struct edge *next;

    



    struct quorem x;

    
    struct quorem dxdy;

    



    struct quorem dxdy_full;

    
    grid_scaled_y_t ytop;

    
    grid_scaled_y_t dy;

    

    grid_scaled_y_t height_left;

    

    int dir;
};


#define EDGE_Y_BUCKET_HEIGHT GRID_Y

#define EDGE_Y_BUCKET_INDEX(y, ymin) (((y) - (ymin))/EDGE_Y_BUCKET_HEIGHT)




struct polygon {
    
    grid_scaled_y_t ymin, ymax;

    


    struct edge **y_buckets;
    struct edge *y_buckets_embedded[64];

    struct {
	struct pool base[1];
	struct edge embedded[32];
    } edge_pool;
};










































struct cell {
    struct cell		*next;
    int			 x;
    grid_area_t		 uncovered_area;
    grid_scaled_y_t	 covered_height;
};




struct cell_list {
    
    struct cell *head;
    
    struct cell tail;

    


    struct cell **cursor;

    

    struct {
	struct pool base[1];
	struct cell embedded[32];
    } cell_pool;
};

struct cell_pair {
    struct cell *cell1;
    struct cell *cell2;
};



struct active_list {
    
    struct edge *head;

    



    grid_scaled_y_t min_height;
};

struct glitter_scan_converter {
    struct polygon	polygon[1];
    struct active_list	active[1];
    struct cell_list	coverages[1];

    
    grid_scaled_x_t xmin, xmax;
    grid_scaled_y_t ymin, ymax;
};



inline static struct quorem
floored_divrem(int a, int b)
{
    struct quorem qr;
    qr.quo = a/b;
    qr.rem = a%b;
    if ((a^b)<0 && qr.rem) {
	qr.quo -= 1;
	qr.rem += b;
    }
    return qr;
}



static struct quorem
floored_muldivrem(int x, int a, int b)
{
    struct quorem qr;
    long long xa = (long long)x*a;
    qr.quo = xa/b;
    qr.rem = xa%b;
    if ((xa>=0) != (b>=0) && qr.rem) {
	qr.quo -= 1;
	qr.rem += b;
    }
    return qr;
}

static void
_pool_chunk_init(
    struct _pool_chunk *p,
    struct _pool_chunk *prev_chunk,
    size_t capacity)
{
    p->prev_chunk = prev_chunk;
    p->size = 0;
    p->capacity = capacity;
}

static struct _pool_chunk *
_pool_chunk_create(
    struct _pool_chunk *prev_chunk,
    size_t size)
{
    struct _pool_chunk *p;
    size_t size_with_head = size + sizeof(struct _pool_chunk);
    if (size_with_head < size)
	return NULL;
    p = malloc(size_with_head);
    if (p)
	_pool_chunk_init(p, prev_chunk, size);
    return p;
}

static void
pool_init(
    struct pool *pool,
    size_t default_capacity,
    size_t embedded_capacity)
{
    pool->current = pool->sentinel;
    pool->first_free = NULL;
    pool->default_capacity = default_capacity;
    _pool_chunk_init(pool->sentinel, NULL, embedded_capacity);
}

static void
pool_fini(struct pool *pool)
{
    struct _pool_chunk *p = pool->current;
    do {
	while (NULL != p) {
	    struct _pool_chunk *prev = p->prev_chunk;
	    if (p != pool->sentinel)
		free(p);
	    p = prev;
	}
	p = pool->first_free;
	pool->first_free = NULL;
    } while (NULL != p);
    pool_init(pool, 0, 0);
}





static void *
_pool_alloc_from_new_chunk(
    struct pool *pool,
    size_t size)
{
    struct _pool_chunk *chunk;
    void *obj;
    size_t capacity;

    


    capacity = size;
    chunk = NULL;
    if (size < pool->default_capacity) {
	capacity = pool->default_capacity;
	chunk = pool->first_free;
	if (chunk) {
	    pool->first_free = chunk->prev_chunk;
	    _pool_chunk_init(chunk, pool->current, chunk->capacity);
	}
    }

    if (NULL == chunk) {
	chunk = _pool_chunk_create(
	    pool->current,
	    capacity);
	if (NULL == chunk)
	    return NULL;
    }
    pool->current = chunk;

    obj = ((unsigned char*)chunk + sizeof(*chunk) + chunk->size);
    chunk->size += size;
    return obj;
}







inline static void *
pool_alloc(
    struct pool *pool,
    size_t size)
{
    struct _pool_chunk *chunk = pool->current;

    if (size <= chunk->capacity - chunk->size) {
	void *obj = ((unsigned char*)chunk + sizeof(*chunk) + chunk->size);
	chunk->size += size;
	return obj;
    }
    else {
	return _pool_alloc_from_new_chunk(pool, size);
    }
}


static void
pool_reset(struct pool *pool)
{
    
    struct _pool_chunk *chunk = pool->current;
    if (chunk != pool->sentinel) {
	while (chunk->prev_chunk != pool->sentinel) {
	    chunk = chunk->prev_chunk;
	}
	chunk->prev_chunk = pool->first_free;
	pool->first_free = pool->current;
    }
    
    pool->current = pool->sentinel;
    pool->sentinel->size = 0;
}



inline static void
cell_list_rewind(struct cell_list *cells)
{
    cells->cursor = &cells->head;
}


inline static void
cell_list_maybe_rewind(struct cell_list *cells, int x)
{
    struct cell *tail = *cells->cursor;
    if (tail->x > x) {
	cell_list_rewind(cells);
    }
}

static void
cell_list_init(struct cell_list *cells)
{
    pool_init(cells->cell_pool.base,
	      256*sizeof(struct cell),
	      sizeof(cells->cell_pool.embedded));
    cells->tail.next = NULL;
    cells->tail.x = INT_MAX;
    cells->head = &cells->tail;
    cell_list_rewind(cells);
}

static void
cell_list_fini(struct cell_list *cells)
{
    pool_fini(cells->cell_pool.base);
    cell_list_init(cells);
}



inline static void
cell_list_reset(struct cell_list *cells)
{
    cell_list_rewind(cells);
    cells->head = &cells->tail;
    pool_reset(cells->cell_pool.base);
}






inline static struct cell *
cell_list_find(struct cell_list *cells, int x)
{
    struct cell **cursor = cells->cursor;
    struct cell *tail;

    while (1) {
	UNROLL3({
	    tail = *cursor;
	    if (tail->x >= x) {
		break;
	    }
	    cursor = &tail->next;
	});
    }
    cells->cursor = cursor;

    if (tail->x == x) {
	return tail;
    } else {
	struct cell *cell = pool_alloc(
	    cells->cell_pool.base,
	    sizeof(struct cell));
	if (NULL == cell)
	    return NULL;
	*cursor = cell;
	cell->next = tail;
	cell->x = x;
	cell->uncovered_area = 0;
	cell->covered_height = 0;
	return cell;
    }
}








inline static struct cell_pair
cell_list_find_pair(struct cell_list *cells, int x1, int x2)
{
    struct cell_pair pair;
    struct cell **cursor = cells->cursor;
    struct cell *cell1;
    struct cell *cell2;
    struct cell *newcell;

    
    while (1) {
	UNROLL3({
	    cell1 = *cursor;
	    if (cell1->x > x1)
		break;
	    if (cell1->x == x1)
		goto found_first;
	    cursor = &cell1->next;
	});
    }

    
    newcell = pool_alloc(
	cells->cell_pool.base,
	sizeof(struct cell));
    if (NULL != newcell) {
	*cursor = newcell;
	newcell->next = cell1;
	newcell->x = x1;
	newcell->uncovered_area = 0;
	newcell->covered_height = 0;
    }
    cell1 = newcell;
 found_first:

    
    while (1) {
	UNROLL3({
	    cell2 = *cursor;
	    if (cell2->x > x2)
		break;
	    if (cell2->x == x2)
		goto found_second;
	    cursor = &cell2->next;
	});
    }

    
    newcell = pool_alloc(
	cells->cell_pool.base,
	sizeof(struct cell));
    if (NULL != newcell) {
	*cursor = newcell;
	newcell->next = cell2;
	newcell->x = x2;
	newcell->uncovered_area = 0;
	newcell->covered_height = 0;
    }
    cell2 = newcell;
 found_second:

    cells->cursor = cursor;
    pair.cell1 = cell1;
    pair.cell2 = cell2;
    return pair;
}



static glitter_status_t
cell_list_add_unbounded_subspan(
    struct cell_list *cells,
    grid_scaled_x_t x)
{
    struct cell *cell;
    int ix, fx;

    GRID_X_TO_INT_FRAC(x, ix, fx);

    cell = cell_list_find(cells, ix);
    if (cell) {
	cell->uncovered_area += 2*fx;
	cell->covered_height++;
	return GLITTER_STATUS_SUCCESS;
    }
    return GLITTER_STATUS_NO_MEMORY;
}


inline static glitter_status_t
cell_list_add_subspan(
    struct cell_list *cells,
    grid_scaled_x_t x1,
    grid_scaled_x_t x2)
{
    int ix1, fx1;
    int ix2, fx2;

    GRID_X_TO_INT_FRAC(x1, ix1, fx1);
    GRID_X_TO_INT_FRAC(x2, ix2, fx2);

    if (ix1 != ix2) {
	struct cell_pair p;
	p = cell_list_find_pair(cells, ix1, ix2);
	if (p.cell1 && p.cell2) {
	    p.cell1->uncovered_area += 2*fx1;
	    ++p.cell1->covered_height;
	    p.cell2->uncovered_area -= 2*fx2;
	    --p.cell2->covered_height;
	    return GLITTER_STATUS_SUCCESS;
	}
    }
    else {
	struct cell *cell = cell_list_find(cells, ix1);
	if (cell) {
	    cell->uncovered_area += 2*(fx1-fx2);
	    return GLITTER_STATUS_SUCCESS;
	}
    }
    return GLITTER_STATUS_NO_MEMORY;
}


















static glitter_status_t
cell_list_render_edge(
    struct cell_list *cells,
    struct edge *edge,
    int sign)
{
    struct quorem x1 = edge->x;
    struct quorem x2 = x1;
    grid_scaled_y_t y1, y2, dy;
    grid_scaled_x_t dx;
    int ix1, ix2;
    grid_scaled_x_t fx1, fx2;

    x2.quo += edge->dxdy_full.quo;
    x2.rem += edge->dxdy_full.rem;
    if (x2.rem >= 0) {
	++x2.quo;
	x2.rem -= edge->dy;
    }
    edge->x = x2;

    GRID_X_TO_INT_FRAC(x1.quo, ix1, fx1);
    GRID_X_TO_INT_FRAC(x2.quo, ix2, fx2);

    
    if (ix1 == ix2) {
	

	struct cell *cell = cell_list_find(cells, ix1);
	if (NULL == cell)
	    return GLITTER_STATUS_NO_MEMORY;
	cell->covered_height += sign*GRID_Y;
	cell->uncovered_area += sign*(fx1 + fx2)*GRID_Y;
	return GLITTER_STATUS_SUCCESS;
    }

    
    dx = x2.quo - x1.quo;
    if (dx >= 0) {
	y1 = 0;
	y2 = GRID_Y;
    } else {
	int tmp;
	tmp = ix1; ix1 = ix2; ix2 = tmp;
	tmp = fx1; fx1 = fx2; fx2 = tmp;
	dx = -dx;
	sign = -sign;
	y1 = GRID_Y;
	y2 = 0;
    }
    dy = y2 - y1;

    

    {
	struct cell_pair pair;
	struct quorem y = floored_divrem((GRID_X - fx1)*dy, dx);

	

















	cell_list_maybe_rewind(cells, ix1);

	pair = cell_list_find_pair(cells, ix1, ix1+1);
	if (!pair.cell1 || !pair.cell2)
	    return GLITTER_STATUS_NO_MEMORY;

	pair.cell1->uncovered_area += sign*y.quo*(GRID_X + fx1);
	pair.cell1->covered_height += sign*y.quo;
	y.quo += y1;

	if (ix1+1 < ix2) {
	    struct quorem dydx_full = floored_divrem(GRID_X*dy, dx);
	    struct cell *cell = pair.cell2;

	    ++ix1;
	    do {
		grid_scaled_y_t y_skip = dydx_full.quo;
		y.rem += dydx_full.rem;
		if (y.rem >= dx) {
		    ++y_skip;
		    y.rem -= dx;
		}

		y.quo += y_skip;

		y_skip *= sign;
		cell->uncovered_area += y_skip*GRID_X;
		cell->covered_height += y_skip;

		++ix1;
		cell = cell_list_find(cells, ix1);
		if (NULL == cell)
		    return GLITTER_STATUS_NO_MEMORY;
	    } while (ix1 != ix2);

	    pair.cell2 = cell;
	}
	pair.cell2->uncovered_area += sign*(y2 - y.quo)*fx2;
	pair.cell2->covered_height += sign*(y2 - y.quo);
    }

    return GLITTER_STATUS_SUCCESS;
}

static void
polygon_init(struct polygon *polygon)
{
    polygon->ymin = polygon->ymax = 0;
    polygon->y_buckets = polygon->y_buckets_embedded;
    pool_init(polygon->edge_pool.base,
	      8192 - sizeof(struct _pool_chunk),
	      sizeof(polygon->edge_pool.embedded));
}

static void
polygon_fini(struct polygon *polygon)
{
    if (polygon->y_buckets != polygon->y_buckets_embedded)
	free(polygon->y_buckets);
    pool_fini(polygon->edge_pool.base);
    polygon_init(polygon);
}




static glitter_status_t
polygon_reset(
    struct polygon *polygon,
    grid_scaled_y_t ymin,
    grid_scaled_y_t ymax)
{
    unsigned h = ymax - ymin;
    unsigned num_buckets = EDGE_Y_BUCKET_INDEX(ymax + EDGE_Y_BUCKET_HEIGHT-1,
					       ymin);

    pool_reset(polygon->edge_pool.base);

    if (h > 0x7FFFFFFFU - EDGE_Y_BUCKET_HEIGHT)
	goto bail_no_mem; 

    if (polygon->y_buckets != polygon->y_buckets_embedded)
	free (polygon->y_buckets);
    polygon->y_buckets =  polygon->y_buckets_embedded;
    if (num_buckets > ARRAY_LENGTH (polygon->y_buckets_embedded)) {
	polygon->y_buckets = _cairo_malloc_ab (num_buckets,
					       sizeof (struct edge *));
	if (unlikely (NULL == polygon->y_buckets))
	    goto bail_no_mem;
    }
    memset (polygon->y_buckets, 0, num_buckets * sizeof (struct edge *));

    polygon->ymin = ymin;
    polygon->ymax = ymax;
    return GLITTER_STATUS_SUCCESS;

 bail_no_mem:
    polygon->ymin = 0;
    polygon->ymax = 0;
    return GLITTER_STATUS_NO_MEMORY;
}

static void
_polygon_insert_edge_into_its_y_bucket(
    struct polygon *polygon,
    struct edge *e)
{
    unsigned ix = EDGE_Y_BUCKET_INDEX(e->ytop, polygon->ymin);
    struct edge **ptail = &polygon->y_buckets[ix];
    e->next = *ptail;
    *ptail = e;
}

inline static glitter_status_t
polygon_add_edge(
    struct polygon *polygon,
    int x0, int y0,
    int x1, int y1,
    int dir)
{
    struct edge *e;
    grid_scaled_x_t dx;
    grid_scaled_y_t dy;
    grid_scaled_y_t ytop, ybot;
    grid_scaled_y_t ymin = polygon->ymin;
    grid_scaled_y_t ymax = polygon->ymax;

    if (y0 == y1)
	return GLITTER_STATUS_SUCCESS;

    if (y0 > y1) {
	int tmp;
	tmp = x0; x0 = x1; x1 = tmp;
	tmp = y0; y0 = y1; y1 = tmp;
	dir = -dir;
    }

    if (y0 >= ymax || y1 <= ymin)
	return GLITTER_STATUS_SUCCESS;

    e = pool_alloc(polygon->edge_pool.base,
		   sizeof(struct edge));
    if (NULL == e)
	return GLITTER_STATUS_NO_MEMORY;

    dx = x1 - x0;
    dy = y1 - y0;
    e->dy = dy;
    e->dxdy = floored_divrem(dx, dy);

    if (ymin <= y0) {
	ytop = y0;
	e->x.quo = x0;
	e->x.rem = 0;
    }
    else {
	ytop = ymin;
	e->x = floored_muldivrem(ymin - y0, dx, dy);
	e->x.quo += x0;
    }

    e->dir = dir;
    e->ytop = ytop;
    ybot = y1 < ymax ? y1 : ymax;
    e->height_left = ybot - ytop;

    if (e->height_left >= GRID_Y) {
	e->dxdy_full = floored_muldivrem(GRID_Y, dx, dy);
    }
    else {
	e->dxdy_full.quo = 0;
	e->dxdy_full.rem = 0;
    }

    _polygon_insert_edge_into_its_y_bucket(polygon, e);

    e->x.rem -= dy;		

    return GLITTER_STATUS_SUCCESS;
}

static void
active_list_reset(
    struct active_list *active)
{
    active->head = NULL;
    active->min_height = 0;
}

static void
active_list_init(struct active_list *active)
{
    active_list_reset(active);
}

static void
active_list_fini(
    struct active_list *active)
{
    active_list_reset(active);
}




static struct edge *
merge_unsorted_edges(struct edge *sorted_head, struct edge *unsorted_head)
{
    struct edge *head = unsorted_head;
    struct edge **cursor = &sorted_head;
    int x;

    while (NULL != head) {
	struct edge *prev = *cursor;
	struct edge *next = head->next;
	x = head->x.quo;

	if (NULL == prev || x < prev->x.quo) {
	    cursor = &sorted_head;
	}

	while (1) {
	    UNROLL3({
		prev = *cursor;
		if (NULL == prev || prev->x.quo >= x)
		    break;
		cursor = &prev->next;
	    });
	}

	head->next = *cursor;
	*cursor = head;

	head = next;
    }
    return sorted_head;
}



inline static int
active_list_can_step_full_row(
    struct active_list *active)
{
    

    if (active->min_height <= 0) {
	struct edge *e = active->head;
	int min_height = INT_MAX;

	while (NULL != e) {
	    if (e->height_left < min_height)
		min_height = e->height_left;
	    e = e->next;
	}

	active->min_height = min_height;
    }

    

    if (active->min_height >= GRID_Y) {
	grid_scaled_x_t prev_x = INT_MIN;
	struct edge *e = active->head;
	while (NULL != e) {
	    struct quorem x = e->x;

	    x.quo += e->dxdy_full.quo;
	    x.rem += e->dxdy_full.rem;
	    if (x.rem >= 0)
		++x.quo;

	    if (x.quo <= prev_x)
		return 0;
	    prev_x = x.quo;
	    e = e->next;
	}
	return 1;
    }
    return 0;
}



inline static void
active_list_merge_edges_from_polygon(
    struct active_list *active,
    grid_scaled_y_t y,
    struct polygon *polygon)
{
    

    unsigned ix = EDGE_Y_BUCKET_INDEX(y, polygon->ymin);
    int min_height = active->min_height;
    struct edge *subrow_edges = NULL;
    struct edge **ptail = &polygon->y_buckets[ix];

    while (1) {
	struct edge *tail = *ptail;
	if (NULL == tail) break;

	if (y == tail->ytop) {
	    *ptail = tail->next;
	    tail->next = subrow_edges;
	    subrow_edges = tail;
	    if (tail->height_left < min_height)
		min_height = tail->height_left;
	}
	else {
	    ptail = &tail->next;
	}
    }
    active->head = merge_unsorted_edges(active->head, subrow_edges);
    active->min_height = min_height;
}



inline static void
active_list_substep_edges(
    struct active_list *active)
{
    struct edge **cursor = &active->head;
    grid_scaled_x_t prev_x = INT_MIN;
    struct edge *unsorted = NULL;

    while (1) {
	struct edge *edge;

	UNROLL3({
	    edge = *cursor;
	    if (NULL == edge)
		break;

	    if (0 != --edge->height_left) {
		edge->x.quo += edge->dxdy.quo;
		edge->x.rem += edge->dxdy.rem;
		if (edge->x.rem >= 0) {
		    ++edge->x.quo;
		    edge->x.rem -= edge->dy;
		}

		if (edge->x.quo < prev_x) {
		    *cursor = edge->next;
		    edge->next = unsorted;
		    unsorted = edge;
		} else {
		    prev_x = edge->x.quo;
		    cursor = &edge->next;
		}

	    } else {
		*cursor = edge->next;
	    }
	});
    }

    if (unsorted)
	active->head = merge_unsorted_edges(active->head, unsorted);
}

inline static glitter_status_t
apply_nonzero_fill_rule_for_subrow(
    struct active_list *active,
    struct cell_list *coverages)
{
    struct edge *edge = active->head;
    int winding = 0;
    int xstart;
    int xend;
    int status;

    cell_list_rewind(coverages);

    while (NULL != edge) {
	xstart = edge->x.quo;
	winding = edge->dir;
	while (1) {
	    edge = edge->next;
	    if (NULL == edge) {
		return cell_list_add_unbounded_subspan(
		    coverages, xstart);
	    }
	    winding += edge->dir;
	    if (0 == winding)
		break;
	}

	xend = edge->x.quo;
	status = cell_list_add_subspan(coverages, xstart, xend);
	if (status)
	    return status;

	edge = edge->next;
    }

    return GLITTER_STATUS_SUCCESS;
}

static glitter_status_t
apply_evenodd_fill_rule_for_subrow(
    struct active_list *active,
    struct cell_list *coverages)
{
    struct edge *edge = active->head;
    int xstart;
    int xend;
    int status;

    cell_list_rewind(coverages);

    while (NULL != edge) {
	xstart = edge->x.quo;

	edge = edge->next;
	if (NULL == edge) {
	    return cell_list_add_unbounded_subspan(
		coverages, xstart);
	}

	xend = edge->x.quo;
	status = cell_list_add_subspan(coverages, xstart, xend);
	if (status)
	    return status;

	edge = edge->next;
    }

    return GLITTER_STATUS_SUCCESS;
}

static glitter_status_t
apply_nonzero_fill_rule_and_step_edges(
    struct active_list *active,
    struct cell_list *coverages)
{
    struct edge **cursor = &active->head;
    struct edge *left_edge;
    int status;

    left_edge = *cursor;
    while (NULL != left_edge) {
	struct edge *right_edge;
	int winding = left_edge->dir;

	left_edge->height_left -= GRID_Y;
	if (left_edge->height_left) {
	    cursor = &left_edge->next;
	}
	else {
	    *cursor = left_edge->next;
	}

	while (1) {
	    right_edge = *cursor;

	    if (NULL == right_edge) {
		return cell_list_render_edge(
		    coverages, left_edge, +1);
	    }

	    right_edge->height_left -= GRID_Y;
	    if (right_edge->height_left) {
		cursor = &right_edge->next;
	    }
	    else {
		*cursor = right_edge->next;
	    }

	    winding += right_edge->dir;
	    if (0 == winding)
		break;

	    right_edge->x.quo += right_edge->dxdy_full.quo;
	    right_edge->x.rem += right_edge->dxdy_full.rem;
	    if (right_edge->x.rem >= 0) {
		++right_edge->x.quo;
		right_edge->x.rem -= right_edge->dy;
	    }
	}

	status = cell_list_render_edge(
	    coverages, left_edge, +1);
	if (status)
	    return status;
	status = cell_list_render_edge(
	    coverages, right_edge, -1);
	if (status)
	    return status;

	left_edge = *cursor;
    }

    return GLITTER_STATUS_SUCCESS;
}

static glitter_status_t
apply_evenodd_fill_rule_and_step_edges(
    struct active_list *active,
    struct cell_list *coverages)
{
    struct edge **cursor = &active->head;
    struct edge *left_edge;
    int status;

    left_edge = *cursor;
    while (NULL != left_edge) {
	struct edge *right_edge;

	left_edge->height_left -= GRID_Y;
	if (left_edge->height_left) {
	    cursor = &left_edge->next;
	}
	else {
	    *cursor = left_edge->next;
	}

	right_edge = *cursor;

	if (NULL == right_edge) {
	    return cell_list_render_edge(
		coverages, left_edge, +1);
	}

	right_edge->height_left -= GRID_Y;
	if (right_edge->height_left) {
	    cursor = &right_edge->next;
	}
	else {
	    *cursor = right_edge->next;
	}

	status = cell_list_render_edge(
	    coverages, left_edge, +1);
	if (status)
	    return status;
	status = cell_list_render_edge(
	    coverages, right_edge, -1);
	if (status)
	    return status;

	left_edge = *cursor;
    }

    return GLITTER_STATUS_SUCCESS;
}



#ifndef GLITTER_BLIT_COVERAGES

inline static void
blit_span(
    unsigned char *row_pixels,
    int x, unsigned len,
    grid_area_t coverage)
{
    int alpha = GRID_AREA_TO_ALPHA(coverage);
    if (1 == len) {
	row_pixels[x] = alpha;
    }
    else {
	memset(row_pixels + x, alpha, len);
    }
}

#define GLITTER_BLIT_COVERAGES(coverages, y, xmin, xmax) \
	blit_cells(coverages, raster_pixels + (y)*raster_stride, xmin, xmax)

static void
blit_cells(
    struct cell_list *cells,
    unsigned char *row_pixels,
    int xmin, int xmax)
{
    struct cell *cell = cells->head;
    int prev_x = xmin;
    int coverage = 0;
    if (NULL == cell)
	return;

    while (NULL != cell && cell->x < xmin) {
	coverage += cell->covered_height;
	cell = cell->next;
    }
    coverage *= GRID_X*2;

    for (; NULL != cell; cell = cell->next) {
	int x = cell->x;
	int area;
	if (x >= xmax)
	    break;
	if (x > prev_x && 0 != coverage) {
	    blit_span(row_pixels, prev_x, x - prev_x, coverage);
	}

	coverage += cell->covered_height * GRID_X*2;
	area = coverage - cell->uncovered_area;
	if (area) {
	    blit_span(row_pixels, x, 1, area);
	}
	prev_x = x+1;
    }

    if (0 != coverage && prev_x < xmax) {
	blit_span(row_pixels, prev_x, xmax - prev_x, coverage);
    }
}
#endif 

static void
_glitter_scan_converter_init(glitter_scan_converter_t *converter)
{
    polygon_init(converter->polygon);
    active_list_init(converter->active);
    cell_list_init(converter->coverages);
    converter->xmin=0;
    converter->ymin=0;
    converter->xmax=0;
    converter->ymax=0;
}

static void
_glitter_scan_converter_fini(glitter_scan_converter_t *converter)
{
    polygon_fini(converter->polygon);
    active_list_fini(converter->active);
    cell_list_fini(converter->coverages);
    converter->xmin=0;
    converter->ymin=0;
    converter->xmax=0;
    converter->ymax=0;
}

static grid_scaled_t
int_to_grid_scaled(int i, int scale)
{
    
    if (i >= 0) {
	if (i >= INT_MAX/scale)
	    i = INT_MAX/scale;
    }
    else {
	if (i <= INT_MIN/scale)
	    i = INT_MIN/scale;
    }
    return i*scale;
}

#define int_to_grid_scaled_x(x) int_to_grid_scaled((x), GRID_X)
#define int_to_grid_scaled_y(x) int_to_grid_scaled((x), GRID_Y)

I glitter_status_t
glitter_scan_converter_reset(
    glitter_scan_converter_t *converter,
    int xmin, int ymin,
    int xmax, int ymax)
{
    glitter_status_t status;

    converter->xmin = 0; converter->xmax = 0;
    converter->ymin = 0; converter->ymax = 0;

    xmin = int_to_grid_scaled_x(xmin);
    ymin = int_to_grid_scaled_y(ymin);
    xmax = int_to_grid_scaled_x(xmax);
    ymax = int_to_grid_scaled_y(ymax);

    active_list_reset(converter->active);
    cell_list_reset(converter->coverages);
    status = polygon_reset(converter->polygon, ymin, ymax);
    if (status)
	return status;

    converter->xmin = xmin;
    converter->xmax = xmax;
    converter->ymin = ymin;
    converter->ymax = ymax;
    return GLITTER_STATUS_SUCCESS;
}








#if !defined(INPUT_TO_GRID_Y) && defined(GRID_Y_BITS) && GRID_Y_BITS <= GLITTER_INPUT_BITS
#  define INPUT_TO_GRID_Y(in, out) (out) = (in) >> (GLITTER_INPUT_BITS - GRID_Y_BITS)
#else
#  define INPUT_TO_GRID_Y(in, out) INPUT_TO_GRID_general(in, out, GRID_Y)
#endif

#if !defined(INPUT_TO_GRID_X) && defined(GRID_X_BITS) && GRID_X_BITS <= GLITTER_INPUT_BITS
#  define INPUT_TO_GRID_X(in, out) (out) = (in) >> (GLITTER_INPUT_BITS - GRID_X_BITS)
#else
#  define INPUT_TO_GRID_X(in, out) INPUT_TO_GRID_general(in, out, GRID_X)
#endif

#define INPUT_TO_GRID_general(in, out, grid_scale) do {		\
	long long tmp__ = (long long)(grid_scale) * (in);	\
	tmp__ >>= GLITTER_INPUT_BITS;				\
	(out) = tmp__;						\
} while (0)

I glitter_status_t
glitter_scan_converter_add_edge(
    glitter_scan_converter_t *converter,
    glitter_input_scaled_t x1, glitter_input_scaled_t y1,
    glitter_input_scaled_t x2, glitter_input_scaled_t y2,
    int dir)
{
    
    grid_scaled_y_t sx1, sy1;
    grid_scaled_y_t sx2, sy2;

    INPUT_TO_GRID_Y(y1, sy1);
    INPUT_TO_GRID_Y(y2, sy2);
    if (sy1 == sy2)
	return GLITTER_STATUS_SUCCESS;

    INPUT_TO_GRID_X(x1, sx1);
    INPUT_TO_GRID_X(x2, sx2);

    return polygon_add_edge(
	converter->polygon, sx1, sy1, sx2, sy2, dir);
}

#ifndef GLITTER_BLIT_COVERAGES_BEGIN
# define GLITTER_BLIT_COVERAGES_BEGIN
#endif

#ifndef GLITTER_BLIT_COVERAGES_END
# define GLITTER_BLIT_COVERAGES_END
#endif

#ifndef GLITTER_BLIT_COVERAGES_EMPTY
# define GLITTER_BLIT_COVERAGES_EMPTY(y, xmin, xmax)
#endif

I glitter_status_t
glitter_scan_converter_render(
    glitter_scan_converter_t *converter,
    int nonzero_fill,
    GLITTER_BLIT_COVERAGES_ARGS)
{
    int i;
    int ymax_i = converter->ymax / GRID_Y;
    int ymin_i = converter->ymin / GRID_Y;
    int xmin_i, xmax_i;
    int h = ymax_i - ymin_i;
    struct polygon *polygon = converter->polygon;
    struct cell_list *coverages = converter->coverages;
    struct active_list *active = converter->active;

    xmin_i = converter->xmin / GRID_X;
    xmax_i = converter->xmax / GRID_X;
    if (xmin_i >= xmax_i)
	return GLITTER_STATUS_SUCCESS;

    
    GLITTER_BLIT_COVERAGES_BEGIN;

    
    for (i=0; i<h; i++) {
	int do_full_step = 0;
	glitter_status_t status = 0;

	

	if (GRID_Y == EDGE_Y_BUCKET_HEIGHT
	    && !polygon->y_buckets[i])
	{
	    if (!active->head) {
		GLITTER_BLIT_COVERAGES_EMPTY(i+ymin_i, xmin_i, xmax_i);
		continue;
	    }
	    do_full_step = active_list_can_step_full_row(active);
	}

	cell_list_reset(coverages);

	if (do_full_step) {
	    
	    if (nonzero_fill) {
		status = apply_nonzero_fill_rule_and_step_edges(
		    active, coverages);
	    }
	    else {
		status = apply_evenodd_fill_rule_and_step_edges(
		    active, coverages);
	    }
	}
	else {
	    
	    grid_scaled_y_t suby;
	    for (suby = 0; suby < GRID_Y; suby++) {
		grid_scaled_y_t y = (i+ymin_i)*GRID_Y + suby;

		active_list_merge_edges_from_polygon(
		    active, y, polygon);

		if (nonzero_fill)
		    status |= apply_nonzero_fill_rule_for_subrow(
			active, coverages);
		else
		    status |= apply_evenodd_fill_rule_for_subrow(
			active, coverages);

		active_list_substep_edges(active);
	    }
	}

	if (status)
	    return status;

	GLITTER_BLIT_COVERAGES(coverages, i+ymin_i, xmin_i, xmax_i);

	if (!active->head) {
	    active->min_height = INT_MAX;
	}
	else {
	    active->min_height -= GRID_Y;
	}
    }

    
    GLITTER_BLIT_COVERAGES_END;

    return GLITTER_STATUS_SUCCESS;
}





static glitter_status_t
blit_with_span_renderer(
    struct cell_list *cells,
    cairo_span_renderer_t *renderer,
    struct pool *span_pool,
    int y,
    int xmin,
    int xmax)
{
    struct cell *cell = cells->head;
    int prev_x = xmin;
    int cover = 0;
    cairo_half_open_span_t *spans;
    unsigned num_spans;
    if (cell == NULL)
	return CAIRO_STATUS_SUCCESS;

    
    while (cell != NULL && cell->x < xmin) {
	cover += cell->covered_height;
	cell = cell->next;
    }
    cover *= GRID_X*2;

    
    {
	struct cell *next = cell;
	num_spans = 0;
	while (next) {
	    next = next->next;
	    ++num_spans;
	}
	num_spans = 2*num_spans + 1;
    }

    
    pool_reset (span_pool);
    spans = pool_alloc (span_pool, sizeof(spans[0])*num_spans);
    if (spans == NULL)
	return GLITTER_STATUS_NO_MEMORY;

    num_spans = 0;

    
    for (; cell != NULL; cell = cell->next) {
	int x = cell->x;
	int area;
	if (x >= xmax)
	    break;

	if (x > prev_x) {
	    spans[num_spans].x = prev_x;
	    spans[num_spans].coverage = GRID_AREA_TO_ALPHA (cover);
	    ++num_spans;
	}

	cover += cell->covered_height*GRID_X*2;
	area = cover - cell->uncovered_area;

	spans[num_spans].x = x;
	spans[num_spans].coverage = GRID_AREA_TO_ALPHA (area);
	++num_spans;

	prev_x = x+1;
    }

    if (prev_x < xmax) {
	spans[num_spans].x = prev_x;
	spans[num_spans].coverage = GRID_AREA_TO_ALPHA (cover);
	++num_spans;
    }

    
    return renderer->render_row (renderer, y, spans, num_spans);
}

struct _cairo_tor_scan_converter {
    cairo_scan_converter_t base;
    glitter_scan_converter_t converter[1];
    cairo_fill_rule_t fill_rule;

    struct {
	struct pool base[1];
	cairo_half_open_span_t embedded[32];
    } span_pool;
};

typedef struct _cairo_tor_scan_converter cairo_tor_scan_converter_t;

static void
_cairo_tor_scan_converter_destroy(void *abstract_converter)
{
    cairo_tor_scan_converter_t *self = abstract_converter;
    if (self == NULL) {
	return;
    }
    _glitter_scan_converter_fini (self->converter);
    pool_fini (self->span_pool.base);
    free(self);
}

static cairo_status_t
_cairo_tor_scan_converter_add_edge(
    void		*abstract_converter,
    cairo_fixed_t	 x1,
    cairo_fixed_t	 y1,
    cairo_fixed_t	 x2,
    cairo_fixed_t	 y2)
{
    cairo_tor_scan_converter_t *self = abstract_converter;
    cairo_status_t status;
    status = glitter_scan_converter_add_edge (
	self->converter,
	x1, y1, x2, y2, +1);
    if (status) {
	return _cairo_scan_converter_set_error (self,
						_cairo_error (status));
    }
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_tor_scan_converter_generate(
    void			*abstract_converter,
    cairo_span_renderer_t	*renderer)
{
    cairo_tor_scan_converter_t *self = abstract_converter;
    cairo_status_t status = glitter_scan_converter_render (
	self->converter,
	self->fill_rule == CAIRO_FILL_RULE_WINDING,
	renderer,
	self->span_pool.base);
    if (status) {
	return _cairo_scan_converter_set_error (self,
						_cairo_error (status));
    }
    return CAIRO_STATUS_SUCCESS;
}

cairo_scan_converter_t *
_cairo_tor_scan_converter_create(
    int			xmin,
    int			ymin,
    int			xmax,
    int			ymax,
    cairo_fill_rule_t	fill_rule)
{
    cairo_status_t status;
    cairo_tor_scan_converter_t *self =
	calloc (1, sizeof(struct _cairo_tor_scan_converter));
    if (self == NULL)
	goto bail_nomem;

    self->base.destroy = &_cairo_tor_scan_converter_destroy;
    self->base.add_edge = &_cairo_tor_scan_converter_add_edge;
    self->base.generate = &_cairo_tor_scan_converter_generate;

    pool_init (self->span_pool.base,
	      250 * sizeof(self->span_pool.embedded[0]),
	      sizeof(self->span_pool.embedded));

    _glitter_scan_converter_init (self->converter);
    status = glitter_scan_converter_reset (
	self->converter, xmin, ymin, xmax, ymax);
    if (status != CAIRO_STATUS_SUCCESS)
	goto bail;

    self->fill_rule = fill_rule;

    return &self->base;

 bail:
    self->base.destroy(&self->base);
 bail_nomem:
    return _cairo_scan_converter_create_in_error (CAIRO_STATUS_NO_MEMORY);
}
