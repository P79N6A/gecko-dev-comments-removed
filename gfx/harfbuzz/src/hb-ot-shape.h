

























#ifndef HB_OT_SHAPE_H
#define HB_OT_SHAPE_H

#include "hb-common.h"
#include "hb-shape.h"


HB_BEGIN_DECLS


hb_bool_t
hb_ot_shape (hb_font_t          *font,
	     hb_buffer_t        *buffer,
	     const hb_feature_t *features,
	     unsigned int        num_features,
	     const char * const *shaper_options);


HB_END_DECLS

#endif 
