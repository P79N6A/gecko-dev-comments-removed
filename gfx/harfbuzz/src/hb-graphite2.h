
























#ifndef HB_GRAPHITE2_H
#define HB_GRAPHITE2_H

#include "hb-common.h"
#include "hb-shape.h"

HB_BEGIN_DECLS


#define HB_GRAPHITE_TAG_Silf HB_TAG('S','i','l','f')

hb_bool_t
hb_graphite_shape (hb_font_t          *font,
		   hb_buffer_t        *buffer,
		   const hb_feature_t *features,
		   unsigned int        num_features,
		   const char * const *shaper_options);

HB_END_DECLS

#endif 
