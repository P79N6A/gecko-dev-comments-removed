


























#ifndef HB_GRAPHITE_H
#define HB_GRAPHITE_H

#include "hb-shape.h"

HB_BEGIN_DECLS


#define HB_GRAPHITE_TAG_Silf HB_TAG('S','i','l','f')

void hb_graphite_shape (hb_font_t    *font,
			hb_face_t    *face,
			hb_buffer_t  *buffer,
			hb_feature_t *features,
			unsigned int  num_features);


HB_END_DECLS

#endif 
