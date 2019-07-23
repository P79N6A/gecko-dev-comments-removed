








































#ifndef CAIRO_PDF_OPERATORS_H
#define CAIRO_PDF_OPERATORS_H

#include "cairo-compiler-private.h"
#include "cairo-types-private.h"






#define PDF_GLYPH_BUFFER_SIZE 200

typedef cairo_status_t (*cairo_pdf_operators_use_font_subset_t) (unsigned int  font_id,
								 unsigned int  subset_id,
								 void         *closure);

typedef struct _cairo_pdf_glyph {
    unsigned int glyph_index;
    double x_position;
    double x_advance;
} cairo_pdf_glyph_t;

typedef struct _cairo_pdf_operators {
    cairo_output_stream_t *stream;
    cairo_matrix_t cairo_to_pdf;
    cairo_scaled_font_subsets_t *font_subsets;
    cairo_pdf_operators_use_font_subset_t use_font_subset;
    void *use_font_subset_closure;
    cairo_bool_t use_actual_text;
    cairo_bool_t in_text_object; 

    
    cairo_bool_t is_new_text_object; 
    unsigned int font_id;
    unsigned int subset_id;
    cairo_matrix_t text_matrix; 
    cairo_matrix_t cairo_to_pdftext; 
    cairo_matrix_t font_matrix_inverse;
    double cur_x; 
    double cur_y;
    int hex_width;
    int num_glyphs;
    cairo_pdf_glyph_t glyphs[PDF_GLYPH_BUFFER_SIZE];

    
    cairo_bool_t         has_line_style;
    double		 line_width;
    cairo_line_cap_t	 line_cap;
    cairo_line_join_t	 line_join;
    double		 miter_limit;
    cairo_bool_t         has_dashes;
} cairo_pdf_operators_t;

cairo_private void
_cairo_pdf_operators_init (cairo_pdf_operators_t       *pdf_operators,
			   cairo_output_stream_t       *stream,
			   cairo_matrix_t 	       *cairo_to_pdf,
			   cairo_scaled_font_subsets_t *font_subsets);

cairo_private cairo_status_t
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
					      cairo_matrix_t 	    *cairo_to_pdf);

cairo_private void
_cairo_pdf_operators_enable_actual_text (cairo_pdf_operators_t *pdf_operators,
					 cairo_bool_t 	  	enable);

cairo_private cairo_status_t
_cairo_pdf_operators_flush (cairo_pdf_operators_t	 *pdf_operators);

cairo_private void
_cairo_pdf_operators_reset (cairo_pdf_operators_t	 *pdf_operators);

cairo_private cairo_int_status_t
_cairo_pdf_operators_clip (cairo_pdf_operators_t 	*pdf_operators,
			   cairo_path_fixed_t		*path,
			   cairo_fill_rule_t		 fill_rule);

cairo_private cairo_int_status_t
_cairo_pdf_operators_emit_stroke_style (cairo_pdf_operators_t	*pdf_operators,
					cairo_stroke_style_t	*style,
					double			 scale);

cairo_private cairo_int_status_t
_cairo_pdf_operators_stroke (cairo_pdf_operators_t 	*pdf_operators,
			     cairo_path_fixed_t		*path,
			     cairo_stroke_style_t	*style,
			     cairo_matrix_t		*ctm,
			     cairo_matrix_t		*ctm_inverse);

cairo_private cairo_int_status_t
_cairo_pdf_operators_fill (cairo_pdf_operators_t 	*pdf_operators,
			   cairo_path_fixed_t		*path,
			   cairo_fill_rule_t	 	fill_rule);

cairo_private cairo_int_status_t
_cairo_pdf_operators_fill_stroke (cairo_pdf_operators_t 	*pdf_operators,
				  cairo_path_fixed_t		*path,
				  cairo_fill_rule_t	 	 fill_rule,
				  cairo_stroke_style_t	        *style,
				  cairo_matrix_t		*ctm,
				  cairo_matrix_t		*ctm_inverse);

cairo_private cairo_int_status_t
_cairo_pdf_operators_show_text_glyphs (cairo_pdf_operators_t	  *pdf_operators,
				       const char                 *utf8,
				       int                         utf8_len,
				       cairo_glyph_t              *glyphs,
				       int                         num_glyphs,
				       const cairo_text_cluster_t *clusters,
				       int                         num_clusters,
				       cairo_text_cluster_flags_t  cluster_flags,
				       cairo_scaled_font_t	  *scaled_font);

#endif 
