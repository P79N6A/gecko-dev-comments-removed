

























#ifndef CAIRO_SPANS_PRIVATE_H
#define CAIRO_SPANS_PRIVATE_H
#include "cairo-types-private.h"
#include "cairo-compiler-private.h"


#define CAIRO_SPANS_UNIT_COVERAGE_BITS 8
#define CAIRO_SPANS_UNIT_COVERAGE ((1 << CAIRO_SPANS_UNIT_COVERAGE_BITS)-1)



typedef struct _cairo_half_open_span {
    
    int x;

    
    int coverage;
} cairo_half_open_span_t;



typedef struct _cairo_span_renderer cairo_span_renderer_t;
struct _cairo_span_renderer {
    
    cairo_status_t status;

    
    cairo_destroy_func_t	destroy;

    

    cairo_warn cairo_status_t
    (*render_rows) (void *abstract_renderer,
		    int y, int height,
		    const cairo_half_open_span_t	*coverages,
		    unsigned num_coverages);

    


    cairo_status_t (*finish) (void *abstract_renderer);
};


typedef struct _cairo_scan_converter cairo_scan_converter_t;
struct _cairo_scan_converter {
    
    cairo_destroy_func_t	destroy;

    
    cairo_status_t (*add_edge) (void		    *abstract_converter,
				const cairo_point_t *p1,
				const cairo_point_t *p2,
				int top, int bottom,
				int dir);

    
    cairo_status_t (*add_polygon) (void		    *abstract_converter,
				   const cairo_polygon_t  *polygon);

    


    cairo_status_t (*generate) (void			*abstract_converter,
				cairo_span_renderer_t	*renderer);

    
    cairo_status_t status;
};



cairo_private cairo_scan_converter_t *
_cairo_tor_scan_converter_create (int			xmin,
				  int			ymin,
				  int			xmax,
				  int			ymax,
				  cairo_fill_rule_t	fill_rule);

typedef struct _cairo_rectangular_scan_converter {
    cairo_scan_converter_t base;

    int xmin, xmax;
    int ymin, ymax;

    struct _cairo_rectangular_scan_converter_chunk {
	struct _cairo_rectangular_scan_converter_chunk *next;
	void *base;
	int count;
	int size;
    } chunks, *tail;
    char buf[CAIRO_STACK_BUFFER_SIZE];
    int num_rectangles;
} cairo_rectangular_scan_converter_t;

cairo_private void
_cairo_rectangular_scan_converter_init (cairo_rectangular_scan_converter_t *self,
					const cairo_rectangle_int_t *extents);

cairo_private cairo_status_t
_cairo_rectangular_scan_converter_add_box (cairo_rectangular_scan_converter_t *self,
					   const cairo_box_t *box,
					   int dir);

typedef struct _cairo_botor_scan_converter {
    cairo_scan_converter_t base;

    cairo_box_t extents;
    cairo_fill_rule_t fill_rule;

    int xmin, xmax;

    struct _cairo_botor_scan_converter_chunk {
	struct _cairo_botor_scan_converter_chunk *next;
	void *base;
	int count;
	int size;
    } chunks, *tail;
    char buf[CAIRO_STACK_BUFFER_SIZE];
    int num_edges;
} cairo_botor_scan_converter_t;

cairo_private void
_cairo_botor_scan_converter_init (cairo_botor_scan_converter_t *self,
				  const cairo_box_t *extents,
				  cairo_fill_rule_t fill_rule);



cairo_private cairo_scan_converter_t *
_cairo_scan_converter_create_in_error (cairo_status_t error);

cairo_private cairo_status_t
_cairo_scan_converter_status (void *abstract_converter);

cairo_private cairo_status_t
_cairo_scan_converter_set_error (void *abstract_converter,
				 cairo_status_t error);

cairo_private cairo_span_renderer_t *
_cairo_span_renderer_create_in_error (cairo_status_t error);

cairo_private cairo_status_t
_cairo_span_renderer_status (void *abstract_renderer);




cairo_private cairo_status_t
_cairo_span_renderer_set_error (void *abstract_renderer,
				cairo_status_t error);

cairo_private cairo_status_t
_cairo_surface_composite_polygon (cairo_surface_t	*surface,
				  cairo_operator_t	 op,
				  const cairo_pattern_t	*pattern,
				  cairo_fill_rule_t	fill_rule,
				  cairo_antialias_t	antialias,
				  const cairo_composite_rectangles_t *rects,
				  cairo_polygon_t	*polygon,
				  cairo_region_t	*clip_region);

#endif 
