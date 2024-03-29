





































#ifndef CAIRO_SURFACE_FALLBACK_PRIVATE_H
#define CAIRO_SURFACE_FALLBACK_PRIVATE_H

#include "cairoint.h"

cairo_private cairo_status_t
_cairo_surface_fallback_paint (cairo_surface_t		*surface,
			       cairo_operator_t		 op,
			       const cairo_pattern_t	*source,
			       cairo_clip_t		*clip);

cairo_private cairo_status_t
_cairo_surface_fallback_mask (cairo_surface_t		*surface,
			      cairo_operator_t		 op,
			      const cairo_pattern_t	*source,
			      const cairo_pattern_t	*mask,
			      cairo_clip_t		*clip);

cairo_private cairo_status_t
_cairo_surface_fallback_stroke (cairo_surface_t		*surface,
				cairo_operator_t	 op,
				const cairo_pattern_t	*source,
				cairo_path_fixed_t	*path,
				const cairo_stroke_style_t	*stroke_style,
				const cairo_matrix_t		*ctm,
				const cairo_matrix_t		*ctm_inverse,
				double			 tolerance,
				cairo_antialias_t	 antialias,
				cairo_clip_t		*clip);

cairo_private cairo_status_t
_cairo_surface_fallback_fill (cairo_surface_t		*surface,
			      cairo_operator_t		 op,
			      const cairo_pattern_t	*source,
			      cairo_path_fixed_t	*path,
			      cairo_fill_rule_t		 fill_rule,
			      double			 tolerance,
			      cairo_antialias_t		 antialias,
			      cairo_clip_t		*clip);

cairo_private cairo_status_t
_cairo_surface_fallback_show_glyphs (cairo_surface_t		*surface,
				     cairo_operator_t		 op,
				     const cairo_pattern_t	*source,
				     cairo_glyph_t		*glyphs,
				     int			 num_glyphs,
				     cairo_scaled_font_t	*scaled_font,
				     cairo_clip_t		*clip);

cairo_private cairo_surface_t *
_cairo_surface_fallback_snapshot (cairo_surface_t *surface);

cairo_private cairo_status_t
_cairo_surface_fallback_composite (cairo_operator_t		 op,
				   const cairo_pattern_t	*src,
				   const cairo_pattern_t	*mask,
				   cairo_surface_t		*dst,
				   int				 src_x,
				   int				 src_y,
				   int				 mask_x,
				   int				 mask_y,
				   int				 dst_x,
				   int				 dst_y,
				   unsigned int			 width,
				   unsigned int			 height,
				   cairo_region_t		*clip_region);

cairo_private cairo_status_t
_cairo_surface_fallback_fill_rectangles (cairo_surface_t         *surface,
					 cairo_operator_t	 op,
					 const cairo_color_t	 *color,
					 cairo_rectangle_int_t   *rects,
					 int			 num_rects);

cairo_private cairo_status_t
_cairo_surface_fallback_composite_trapezoids (cairo_operator_t		op,
					      const cairo_pattern_t    *pattern,
					      cairo_surface_t	       *dst,
					      cairo_antialias_t		antialias,
					      int			src_x,
					      int			src_y,
					      int			dst_x,
					      int			dst_y,
					      unsigned int		width,
					      unsigned int		height,
					      cairo_trapezoid_t	       *traps,
					      int			num_traps,
					      cairo_region_t		*clip_region);

cairo_private cairo_status_t
_cairo_surface_fallback_clone_similar (cairo_surface_t  *surface,
				       cairo_surface_t  *src,
				       int               src_x,
				       int               src_y,
				       int               width,
				       int               height,
				       int              *clone_offset_x,
				       int              *clone_offset_y,
				       cairo_surface_t **clone_out);

#endif
