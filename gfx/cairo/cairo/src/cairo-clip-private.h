


































#ifndef CAIRO_CLIP_PRIVATE_H
#define CAIRO_CLIP_PRIVATE_H

#include "cairo-path-fixed-private.h"

extern const cairo_private cairo_rectangle_list_t _cairo_rectangles_nil;

struct _cairo_clip_path {
    unsigned int	ref_count;
    cairo_path_fixed_t	path;
    cairo_fill_rule_t	fill_rule;
    double		tolerance;
    cairo_antialias_t	antialias;
    cairo_clip_path_t	*prev;
};

struct _cairo_clip {
    cairo_clip_mode_t mode;

    









    cairo_surface_t *surface;
    cairo_rectangle_int_t surface_rect;
    



    unsigned int serial;
    


    cairo_region_t region;
    cairo_bool_t has_region;
    



    cairo_clip_path_t *path;
};

cairo_private void
_cairo_clip_init (cairo_clip_t *clip, cairo_surface_t *target);

cairo_private cairo_status_t
_cairo_clip_init_copy (cairo_clip_t *clip, cairo_clip_t *other);

cairo_private cairo_status_t
_cairo_clip_init_deep_copy (cairo_clip_t    *clip,
                            cairo_clip_t    *other,
                            cairo_surface_t *target);

cairo_private void
_cairo_clip_reset (cairo_clip_t *clip);

cairo_private cairo_status_t
_cairo_clip_clip (cairo_clip_t       *clip,
		  cairo_path_fixed_t *path,
		  cairo_fill_rule_t   fill_rule,
		  double              tolerance,
		  cairo_antialias_t   antialias,
		  cairo_surface_t    *target);

cairo_private cairo_status_t
_cairo_clip_intersect_to_rectangle (cairo_clip_t            *clip,
				    cairo_rectangle_int_t   *rectangle);

cairo_private cairo_status_t
_cairo_clip_intersect_to_region (cairo_clip_t      *clip,
				 cairo_region_t *region);

cairo_private cairo_status_t
_cairo_clip_combine_to_surface (cairo_clip_t                  *clip,
				cairo_operator_t               op,
				cairo_surface_t               *dst,
				int                            dst_x,
				int                            dst_y,
				const cairo_rectangle_int_t   *extents);

cairo_private void
_cairo_clip_translate (cairo_clip_t  *clip,
                       cairo_fixed_t  tx,
                       cairo_fixed_t  ty);

cairo_private cairo_rectangle_list_t*
_cairo_clip_copy_rectangle_list (cairo_clip_t *clip, cairo_gstate_t *gstate);

#endif 
