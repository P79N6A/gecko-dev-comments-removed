

























#ifndef HB_OT_SHAPE_NORMALIZE_PRIVATE_HH
#define HB_OT_SHAPE_NORMALIZE_PRIVATE_HH

#include "hb-private.hh"

#include "hb-font.h"
#include "hb-buffer.h"


#define glyph_index()	var1.u32

enum hb_ot_shape_normalization_mode_t {
  HB_OT_SHAPE_NORMALIZATION_MODE_DECOMPOSED,
  HB_OT_SHAPE_NORMALIZATION_MODE_COMPOSED_DIACRITICS, 
  HB_OT_SHAPE_NORMALIZATION_MODE_COMPOSED_FULL, 

  HB_OT_SHAPE_NORMALIZATION_MODE_DEFAULT = HB_OT_SHAPE_NORMALIZATION_MODE_COMPOSED_DIACRITICS
};

HB_INTERNAL void _hb_ot_shape_normalize (hb_font_t *font,
					 hb_buffer_t *buffer,
					 hb_ot_shape_normalization_mode_t mode);

#endif 
