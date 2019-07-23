



































#ifndef CAIRO_SCALED_FONT_SUBSETS_PRIVATE_H
#define CAIRO_SCALED_FONT_SUBSETS_PRIVATE_H

#include "cairoint.h"

typedef struct _cairo_scaled_font_subsets cairo_scaled_font_subsets_t;

typedef struct _cairo_scaled_font_subset {
    cairo_scaled_font_t *scaled_font;
    unsigned int font_id;
    unsigned int subset_id;

    


    unsigned long *glyphs;
    unsigned int num_glyphs;
} cairo_scaled_font_subset_t;


















cairo_private cairo_scaled_font_subsets_t *
_cairo_scaled_font_subsets_create (int max_glyphs_per_subset);







cairo_private void
_cairo_scaled_font_subsets_destroy (cairo_scaled_font_subsets_t *font_subsets);



















































cairo_private cairo_status_t
_cairo_scaled_font_subsets_map_glyph (cairo_scaled_font_subsets_t	*font_subsets,
				      cairo_scaled_font_t		*scaled_font,
				      unsigned long			 scaled_font_glyph_index,
				      unsigned int			*font_id_ret,
				      unsigned int			*subset_id_ret,
				      unsigned int			*subset_glyph_index_ret);

typedef void
(*cairo_scaled_font_subset_callback_func_t) (cairo_scaled_font_subset_t	*font_subset,
					     void			*closure);































cairo_private cairo_status_t
_cairo_scaled_font_subsets_foreach (cairo_scaled_font_subsets_t			*font_subsets,
				    cairo_scaled_font_subset_callback_func_t	 font_subset_callback,
				    void					*closure);

typedef struct _cairo_cff_subset {
    char *base_font;
    int *widths;
    long x_min, y_min, x_max, y_max;
    long ascent, descent;
    char *data;
    unsigned long data_length;
} cairo_cff_subset_t;

















cairo_private cairo_status_t
_cairo_cff_subset_init (cairo_cff_subset_t          *cff_subset,
                        const char                  *name,
                        cairo_scaled_font_subset_t  *font_subset);









cairo_private void
_cairo_cff_subset_fini (cairo_cff_subset_t *cff_subset);

typedef struct _cairo_truetype_subset {
    char *base_font;
    int *widths;
    long x_min, y_min, x_max, y_max;
    long ascent, descent;
    char *data;
    unsigned long data_length;
    unsigned long *string_offsets;
    unsigned long num_string_offsets;
} cairo_truetype_subset_t;

















cairo_private cairo_status_t
_cairo_truetype_subset_init (cairo_truetype_subset_t    *truetype_subset,
			     cairo_scaled_font_subset_t	*font_subset);









cairo_private void
_cairo_truetype_subset_fini (cairo_truetype_subset_t *truetype_subset);



typedef struct _cairo_type1_subset {
    char *base_font;
    int *widths;
    long x_min, y_min, x_max, y_max;
    long ascent, descent;
    char *data;
    unsigned long header_length;
    unsigned long data_length;
    unsigned long trailer_length;
} cairo_type1_subset_t;

















cairo_private cairo_status_t
_cairo_type1_subset_init (cairo_type1_subset_t		*type_subset,
			  const char			*name,
			  cairo_scaled_font_subset_t	*font_subset,
                          cairo_bool_t                   hex_encode);









cairo_private void
_cairo_type1_subset_fini (cairo_type1_subset_t *subset);

















cairo_private cairo_status_t
_cairo_type1_fallback_init_binary (cairo_type1_subset_t	      *type_subset,
                                   const char		      *name,
                                   cairo_scaled_font_subset_t *font_subset);

















cairo_private cairo_status_t
_cairo_type1_fallback_init_hex (cairo_type1_subset_t	   *type_subset,
                                const char		   *name,
                                cairo_scaled_font_subset_t *font_subset);









cairo_private void
_cairo_type1_fallback_fini (cairo_type1_subset_t *subset);

#endif 
