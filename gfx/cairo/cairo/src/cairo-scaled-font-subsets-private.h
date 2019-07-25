



































#ifndef CAIRO_SCALED_FONT_SUBSETS_PRIVATE_H
#define CAIRO_SCALED_FONT_SUBSETS_PRIVATE_H

#include "cairoint.h"

#if CAIRO_HAS_FONT_SUBSET

typedef struct _cairo_scaled_font_subsets_glyph {
    unsigned int font_id;
    unsigned int subset_id;
    unsigned int subset_glyph_index;
    cairo_bool_t is_scaled;
    cairo_bool_t is_composite;
    double       x_advance;
    double       y_advance;
    cairo_bool_t utf8_is_mapped;
    uint32_t 	 unicode;
} cairo_scaled_font_subsets_glyph_t;















cairo_private cairo_scaled_font_subsets_t *
_cairo_scaled_font_subsets_create_scaled (void);



















cairo_private cairo_scaled_font_subsets_t *
_cairo_scaled_font_subsets_create_simple (void);






















cairo_private cairo_scaled_font_subsets_t *
_cairo_scaled_font_subsets_create_composite (void);







cairo_private void
_cairo_scaled_font_subsets_destroy (cairo_scaled_font_subsets_t *font_subsets);












































































cairo_private cairo_status_t
_cairo_scaled_font_subsets_map_glyph (cairo_scaled_font_subsets_t	*font_subsets,
				      cairo_scaled_font_t		*scaled_font,
				      unsigned long			 scaled_font_glyph_index,
				      const char * 			 utf8,
				      int				 utf8_len,
                                      cairo_scaled_font_subsets_glyph_t *subset_glyph_ret);

typedef cairo_status_t
(*cairo_scaled_font_subset_callback_func_t) (cairo_scaled_font_subset_t	*font_subset,
					     void			*closure);































cairo_private cairo_status_t
_cairo_scaled_font_subsets_foreach_scaled (cairo_scaled_font_subsets_t		    *font_subsets,
				           cairo_scaled_font_subset_callback_func_t  font_subset_callback,
				           void					    *closure);































cairo_private cairo_status_t
_cairo_scaled_font_subsets_foreach_unscaled (cairo_scaled_font_subsets_t              *font_subsets,
                                             cairo_scaled_font_subset_callback_func_t  font_subset_callback,
				             void				      *closure);































cairo_private cairo_status_t
_cairo_scaled_font_subsets_foreach_user (cairo_scaled_font_subsets_t		  *font_subsets,
					 cairo_scaled_font_subset_callback_func_t  font_subset_callback,
					 void					  *closure);













cairo_private cairo_int_status_t
_cairo_scaled_font_subset_create_glyph_names (cairo_scaled_font_subset_t *subset);

typedef struct _cairo_cff_subset {
    char *font_name;
    char *ps_name;
    double *widths;
    double x_min, y_min, x_max, y_max;
    double ascent, descent;
    char *data;
    unsigned long data_length;
} cairo_cff_subset_t;

















cairo_private cairo_status_t
_cairo_cff_subset_init (cairo_cff_subset_t          *cff_subset,
                        const char                  *name,
                        cairo_scaled_font_subset_t  *font_subset);









cairo_private void
_cairo_cff_subset_fini (cairo_cff_subset_t *cff_subset);
















cairo_private cairo_status_t
_cairo_cff_fallback_init (cairo_cff_subset_t          *cff_subset,
                          const char                  *name,
                          cairo_scaled_font_subset_t  *font_subset);









cairo_private void
_cairo_cff_fallback_fini (cairo_cff_subset_t *cff_subset);

typedef struct _cairo_truetype_subset {
    char *font_name;
    char *ps_name;
    double *widths;
    double x_min, y_min, x_max, y_max;
    double ascent, descent;
    unsigned char *data;
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
    double *widths;
    double x_min, y_min, x_max, y_max;
    double ascent, descent;
    char *data;
    unsigned long header_length;
    unsigned long data_length;
    unsigned long trailer_length;
} cairo_type1_subset_t;


#if CAIRO_HAS_FT_FONT

















cairo_private cairo_status_t
_cairo_type1_subset_init (cairo_type1_subset_t		*type_subset,
			  const char			*name,
			  cairo_scaled_font_subset_t	*font_subset,
                          cairo_bool_t                   hex_encode);









cairo_private void
_cairo_type1_subset_fini (cairo_type1_subset_t *subset);

#endif 








cairo_private cairo_bool_t
_cairo_type1_scaled_font_is_type1 (cairo_scaled_font_t	*scaled_font);

















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

typedef struct _cairo_type2_charstrings {
    int *widths;
    long x_min, y_min, x_max, y_max;
    long ascent, descent;
    cairo_array_t charstrings;
} cairo_type2_charstrings_t;
















cairo_private cairo_status_t
_cairo_type2_charstrings_init (cairo_type2_charstrings_t   *charstrings,
                               cairo_scaled_font_subset_t  *font_subset);









cairo_private void
_cairo_type2_charstrings_fini (cairo_type2_charstrings_t *charstrings);















cairo_private cairo_int_status_t
_cairo_truetype_create_glyph_to_unicode_map (cairo_scaled_font_subset_t	*font_subset);


















cairo_private cairo_int_status_t
_cairo_truetype_index_to_ucs4 (cairo_scaled_font_t *scaled_font,
                               unsigned long        index,
                               uint32_t            *ucs4);






















cairo_private cairo_int_status_t
_cairo_truetype_read_font_name (cairo_scaled_font_t   *scaled_font,
				char		     **ps_name,
				char		     **font_name);

#endif 

#endif 
