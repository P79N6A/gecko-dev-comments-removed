

























#ifndef HB_UNISCRIBE_PRIVATE_HH
#define HB_UNISCRIBE_PRIVATE_HH

#include "hb-private.hh"

#include "hb-uniscribe.h"


HB_INTERNAL hb_bool_t
_hb_uniscribe_shape (hb_font_t          *font,
		     hb_buffer_t        *buffer,
		     const hb_feature_t *features,
		     unsigned int        num_features);


#endif 
