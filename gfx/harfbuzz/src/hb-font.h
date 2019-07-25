

























#ifndef HB_FONT_H
#define HB_FONT_H

#include "hb-common.h"
#include "hb-blob.h"

HB_BEGIN_DECLS

typedef struct _hb_face_t hb_face_t;
typedef struct _hb_font_t hb_font_t;





hb_face_t *
hb_face_create_for_data (hb_blob_t    *blob,
			 unsigned int  index);

typedef hb_blob_t * (*hb_get_table_func_t)  (hb_tag_t tag, void *user_data);


hb_face_t *
hb_face_create_for_tables (hb_get_table_func_t  get_table,
			   hb_destroy_func_t    destroy,
			   void                *user_data);

hb_face_t *
hb_face_reference (hb_face_t *face);

unsigned int
hb_face_get_reference_count (hb_face_t *face);

void
hb_face_destroy (hb_face_t *face);


hb_blob_t *
hb_face_get_table (hb_face_t *face,
		   hb_tag_t   tag);






typedef struct _hb_font_funcs_t hb_font_funcs_t;

hb_font_funcs_t *
hb_font_funcs_create (void);

hb_font_funcs_t *
hb_font_funcs_reference (hb_font_funcs_t *ffuncs);

unsigned int
hb_font_funcs_get_reference_count (hb_font_funcs_t *ffuncs);

void
hb_font_funcs_destroy (hb_font_funcs_t *ffuncs);

hb_font_funcs_t *
hb_font_funcs_copy (hb_font_funcs_t *ffuncs);

void
hb_font_funcs_make_immutable (hb_font_funcs_t *ffuncs);




typedef struct _hb_glyph_metrics_t
{
    hb_position_t x_advance;
    hb_position_t y_advance;
    hb_position_t x_offset;
    hb_position_t y_offset;
    hb_position_t width;
    hb_position_t height;
} hb_glyph_metrics_t;

typedef hb_codepoint_t (*hb_font_get_glyph_func_t) (hb_font_t *font, hb_face_t *face, const void *user_data,
						    hb_codepoint_t unicode, hb_codepoint_t variation_selector);
typedef hb_bool_t (*hb_font_get_contour_point_func_t) (hb_font_t *font, hb_face_t *face, const void *user_data,
						       unsigned int point_index,
						       hb_codepoint_t glyph, hb_position_t *x, hb_position_t *y);
typedef void (*hb_font_get_glyph_metrics_func_t) (hb_font_t *font, hb_face_t *face, const void *user_data,
						  hb_codepoint_t glyph, hb_glyph_metrics_t *metrics);
typedef hb_position_t (*hb_font_get_kerning_func_t) (hb_font_t *font, hb_face_t *face, const void *user_data,
						     hb_codepoint_t first_glyph, hb_codepoint_t second_glyph);


void
hb_font_funcs_set_glyph_func (hb_font_funcs_t *ffuncs,
			      hb_font_get_glyph_func_t glyph_func);

void
hb_font_funcs_set_contour_point_func (hb_font_funcs_t *ffuncs,
				      hb_font_get_contour_point_func_t contour_point_func);

void
hb_font_funcs_set_glyph_metrics_func (hb_font_funcs_t *ffuncs,
				      hb_font_get_glyph_metrics_func_t glyph_metrics_func);

void
hb_font_funcs_set_kerning_func (hb_font_funcs_t *ffuncs,
				hb_font_get_kerning_func_t kerning_func);


hb_codepoint_t
hb_font_get_glyph (hb_font_t *font, hb_face_t *face,
		   hb_codepoint_t unicode, hb_codepoint_t variation_selector);

hb_bool_t
hb_font_get_contour_point (hb_font_t *font, hb_face_t *face,
			   unsigned int point_index,
			   hb_codepoint_t glyph, hb_position_t *x, hb_position_t *y);

void
hb_font_get_glyph_metrics (hb_font_t *font, hb_face_t *face,
			   hb_codepoint_t glyph, hb_glyph_metrics_t *metrics);

hb_position_t
hb_font_get_kerning (hb_font_t *font, hb_face_t *face,
		     hb_codepoint_t first_glyph, hb_codepoint_t second_glyph);








hb_font_t *
hb_font_create (void);

hb_font_t *
hb_font_reference (hb_font_t *font);

unsigned int
hb_font_get_reference_count (hb_font_t *font);

void
hb_font_destroy (hb_font_t *font);

void
hb_font_set_funcs (hb_font_t         *font,
		   hb_font_funcs_t   *klass,
		   hb_destroy_func_t  destroy,
		   void              *user_data);

hb_font_funcs_t *
hb_font_get_funcs (hb_font_t       *font);





void
hb_font_set_scale (hb_font_t *font,
		   unsigned int x_scale,
		   unsigned int y_scale);




void
hb_font_set_ppem (hb_font_t *font,
		  unsigned int x_ppem,
		  unsigned int y_ppem);


HB_END_DECLS

#endif 
