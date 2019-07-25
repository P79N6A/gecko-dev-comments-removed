

























#ifndef HB_OT_SHAPE_H
#define HB_OT_SHAPE_H

#include "hb-shape.h"


HB_BEGIN_DECLS

void
hb_ot_shape (hb_font_t    *font,
	     hb_face_t    *face,
	     hb_buffer_t  *buffer,
	     hb_feature_t *features,
	     unsigned int  num_features);

HB_END_DECLS

#endif 
