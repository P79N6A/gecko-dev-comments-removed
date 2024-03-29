



































#ifndef CAIRO_TYPE3_GLYPH_SURFACE_PRIVATE_H
#define CAIRO_TYPE3_GLYPH_SURFACE_PRIVATE_H

#include "cairoint.h"

#if CAIRO_HAS_FONT_SUBSET

#include "cairo-surface-private.h"
#include "cairo-surface-clipper-private.h"
#include "cairo-pdf-operators-private.h"

typedef cairo_status_t (*cairo_type3_glyph_surface_emit_image_t) (cairo_image_surface_t *image,
								  cairo_output_stream_t	*stream);

typedef struct cairo_type3_glyph_surface {
    cairo_surface_t base;

    cairo_scaled_font_t *scaled_font;
    cairo_output_stream_t *stream;
    cairo_pdf_operators_t pdf_operators;
    cairo_matrix_t cairo_to_pdf;
    cairo_type3_glyph_surface_emit_image_t emit_image;

    cairo_surface_clipper_t clipper;
} cairo_type3_glyph_surface_t;

cairo_private cairo_surface_t *
_cairo_type3_glyph_surface_create (cairo_scaled_font_t			 *scaled_font,
				   cairo_output_stream_t		 *stream,
				   cairo_type3_glyph_surface_emit_image_t emit_image,
				   cairo_scaled_font_subsets_t		 *font_subsets);

cairo_private void
_cairo_type3_glyph_surface_set_font_subsets_callback (void				    *abstract_surface,
						      cairo_pdf_operators_use_font_subset_t  use_font_subset,
						      void				    *closure);

cairo_private cairo_status_t
_cairo_type3_glyph_surface_analyze_glyph (void		     *abstract_surface,
					  unsigned long	      glyph_index);

cairo_private cairo_status_t
_cairo_type3_glyph_surface_emit_glyph (void		     *abstract_surface,
				       cairo_output_stream_t *stream,
				       unsigned long	      glyph_index,
				       cairo_box_t           *bbox,
				       double                *width);

#endif 

#endif 
