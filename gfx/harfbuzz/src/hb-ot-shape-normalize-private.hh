

























#ifndef HB_OT_SHAPE_NORMALIZE_PRIVATE_HH
#define HB_OT_SHAPE_NORMALIZE_PRIVATE_HH

#include "hb-private.hh"

#include "hb-font.h"
#include "hb-buffer.h"


#define glyph_index()	var1.u32

struct hb_ot_shape_plan_t;

enum hb_ot_shape_normalization_mode_t {
  HB_OT_SHAPE_NORMALIZATION_MODE_DECOMPOSED,
  HB_OT_SHAPE_NORMALIZATION_MODE_COMPOSED_DIACRITICS, 
  HB_OT_SHAPE_NORMALIZATION_MODE_COMPOSED_DIACRITICS_NO_SHORT_CIRCUIT, 
  HB_OT_SHAPE_NORMALIZATION_MODE_COMPOSED_FULL, 

  HB_OT_SHAPE_NORMALIZATION_MODE_DEFAULT = HB_OT_SHAPE_NORMALIZATION_MODE_COMPOSED_DIACRITICS
};

HB_INTERNAL void _hb_ot_shape_normalize (const hb_ot_shape_plan_t *shaper,
					 hb_buffer_t *buffer,
					 hb_font_t *font);


struct hb_ot_shape_normalize_context_t
{
  const hb_ot_shape_plan_t *plan;
  hb_buffer_t *buffer;
  hb_font_t *font;
  hb_unicode_funcs_t *unicode;
  bool (*decompose) (const hb_ot_shape_normalize_context_t *c,
		     hb_codepoint_t  ab,
		     hb_codepoint_t *a,
		     hb_codepoint_t *b);
  bool (*compose) (const hb_ot_shape_normalize_context_t *c,
		   hb_codepoint_t  a,
		   hb_codepoint_t  b,
		   hb_codepoint_t *ab);
};


#endif 
