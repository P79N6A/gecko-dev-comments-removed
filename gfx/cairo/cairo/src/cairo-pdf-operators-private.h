








































#ifndef CAIRO_PDF_OPERATORS_H
#define CAIRO_PDF_OPERATORS_H

#include "cairo-compiler-private.h"
#include "cairo-types-private.h"

typedef cairo_status_t
(*cairo_pdf_operators_use_font_subset_t) (unsigned int  font_id,
					  unsigned int  subset_id,
					  void         *closure);

typedef struct _cairo_pdf_operators {
    cairo_output_stream_t *stream;
    cairo_matrix_t cairo_to_pdf;
    cairo_scaled_font_subsets_t *font_subsets;
    cairo_pdf_operators_use_font_subset_t use_font_subset;
    void *use_font_subset_closure;
} cairo_pdf_operators_t;

cairo_private void
_cairo_pdf_operators_init (cairo_pdf_operators_t       *pdf_operators,
			   cairo_output_stream_t       *stream,
			   cairo_matrix_t 		cairo_to_pdf,
			   cairo_scaled_font_subsets_t *font_subsets);

cairo_private void
_cairo_pdf_operators_fini (cairo_pdf_operators_t       *pdf_operators);

cairo_private void
_cairo_pdf_operators_set_font_subsets_callback (cairo_pdf_operators_t 		     *pdf_operators,
						cairo_pdf_operators_use_font_subset_t use_font_subset,
						void				     *closure);

cairo_private void
_cairo_pdf_operators_set_stream (cairo_pdf_operators_t 	 *pdf_operators,
				 cairo_output_stream_t   *stream);


cairo_private void
_cairo_pdf_operators_set_cairo_to_pdf_matrix (cairo_pdf_operators_t *pdf_operators,
					      cairo_matrix_t 	     cairo_to_pdf);

cairo_private cairo_int_status_t
_cairo_pdf_operators_clip (cairo_pdf_operators_t 	*pdf_operators,
			   cairo_path_fixed_t		*path,
			   cairo_fill_rule_t		 fill_rule);

cairo_private cairo_int_status_t
_cairo_pdf_operator_stroke (cairo_pdf_operators_t 	*pdf_operators,
			    cairo_path_fixed_t		*path,
			    cairo_stroke_style_t	*style,
			    cairo_matrix_t		*ctm,
			    cairo_matrix_t		*ctm_inverse);

cairo_private cairo_int_status_t
_cairo_pdf_operators_fill (cairo_pdf_operators_t 	*pdf_operators,
			   cairo_path_fixed_t		*path,
			   cairo_fill_rule_t	 	fill_rule);

cairo_private cairo_int_status_t
_cairo_pdf_operators_show_glyphs (cairo_pdf_operators_t *pdf_operators,
				  cairo_glyph_t		*glyphs,
				  int			 num_glyphs,
				  cairo_scaled_font_t	*scaled_font);

#endif 
