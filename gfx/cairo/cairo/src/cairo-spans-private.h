

























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
    
    cairo_destroy_func_t	destroy;

    


    cairo_status_t (*render_row)(
	void				*abstract_renderer,
	int				 y,
	const cairo_half_open_span_t	*coverages,
	unsigned			 num_coverages);

    


    cairo_status_t (*finish)(
	void		      *abstract_renderer);

    
    cairo_status_t status;
};


typedef struct _cairo_scan_converter cairo_scan_converter_t;
struct _cairo_scan_converter {
    
    cairo_destroy_func_t	destroy;

    
    cairo_status_t
    (*add_edge)(
	void		*abstract_converter,
	cairo_fixed_t	 x1,
	cairo_fixed_t	 y1,
	cairo_fixed_t	 x2,
	cairo_fixed_t	 y2);

    


    cairo_status_t
    (*generate)(
	void			*abstract_converter,
	cairo_span_renderer_t	*renderer);

    
    cairo_status_t status;
};



cairo_private cairo_scan_converter_t *
_cairo_tor_scan_converter_create(
    int			xmin,
    int			ymin,
    int			xmax,
    int			ymax,
    cairo_fill_rule_t	fill_rule);



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
_cairo_path_fixed_fill_using_spans (
    cairo_operator_t		 op,
    const cairo_pattern_t	*pattern,
    cairo_path_fixed_t		*path,
    cairo_surface_t		*dst,
    cairo_fill_rule_t		 fill_rule,
    double			 tolerance,
    cairo_antialias_t		 antialias,
    const cairo_composite_rectangles_t *rects);
#endif 
