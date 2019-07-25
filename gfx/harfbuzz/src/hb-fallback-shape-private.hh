

























#ifndef HB_FALLBACK_SHAPE_PRIVATE_HH
#define HB_FALLBACK_SHAPE_PRIVATE_HH

#include "hb-private.hh"

#include "hb-shape.h"


HB_BEGIN_DECLS


HB_INTERNAL hb_bool_t
hb_fallback_shape (hb_font_t          *font,
		   hb_buffer_t        *buffer,
		   const hb_feature_t *features,
		   unsigned int        num_features,
		   const char * const *shaper_options);


HB_END_DECLS

#endif 
