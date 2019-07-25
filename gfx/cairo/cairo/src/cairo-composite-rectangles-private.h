



































#ifndef CAIRO_COMPOSITE_RECTANGLES_PRIVATE_H
#define CAIRO_COMPOSITE_RECTANGLES_PRIVATE_H

#include "cairo-types-private.h"

CAIRO_BEGIN_DECLS










struct _cairo_composite_rectangles {
    cairo_rectangle_int_t source;
    cairo_rectangle_int_t mask;
    cairo_rectangle_int_t bounded; 
    cairo_rectangle_int_t unbounded; 
    uint32_t is_bounded;
};

cairo_private cairo_int_status_t
_cairo_composite_rectangles_init_for_paint (cairo_composite_rectangles_t *extents,
					 const cairo_rectangle_int_t *surface_extents,
					 cairo_operator_t	 op,
					 const cairo_pattern_t	*source,
					 cairo_clip_t		*clip);

cairo_private cairo_int_status_t
_cairo_composite_rectangles_init_for_mask (cairo_composite_rectangles_t *extents,
					const cairo_rectangle_int_t *surface_extents,
					cairo_operator_t	 op,
					const cairo_pattern_t	*source,
					const cairo_pattern_t	*mask,
					cairo_clip_t		*clip);

cairo_private cairo_int_status_t
_cairo_composite_rectangles_init_for_stroke (cairo_composite_rectangles_t *extents,
					     const cairo_rectangle_int_t *surface_extents,
					     cairo_operator_t	 op,
					     const cairo_pattern_t	*source,
					     cairo_path_fixed_t	*path,
					     const cairo_stroke_style_t	*style,
					     const cairo_matrix_t	*ctm,
					     cairo_clip_t		*clip);

cairo_private cairo_int_status_t
_cairo_composite_rectangles_init_for_fill (cairo_composite_rectangles_t *extents,
					   const cairo_rectangle_int_t *surface_extents,
					   cairo_operator_t	 op,
					   const cairo_pattern_t	*source,
					   cairo_path_fixed_t	*path,
					   cairo_clip_t		*clip);

cairo_private cairo_int_status_t
_cairo_composite_rectangles_init_for_glyphs (cairo_composite_rectangles_t *extents,
					     const cairo_rectangle_int_t *surface_extents,
					     cairo_operator_t		 op,
					     const cairo_pattern_t	*source,
					     cairo_scaled_font_t	*scaled_font,
					     cairo_glyph_t		*glyphs,
					     int			 num_glyphs,
					     cairo_clip_t		*clip,
					     cairo_bool_t		*overlap);

#endif 
