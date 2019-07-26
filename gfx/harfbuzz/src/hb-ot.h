

























#ifndef HB_OT_H
#define HB_OT_H
#define HB_OT_H_IN

#include "hb.h"

#include "hb-ot-layout.h"
#include "hb-ot-tag.h"

HB_BEGIN_DECLS


void
hb_ot_shape_glyphs_closure (hb_font_t          *font,
			    hb_buffer_t        *buffer,
			    const hb_feature_t *features,
			    unsigned int        num_features,
			    hb_set_t           *glyphs);

HB_END_DECLS

#undef HB_OT_H_IN
#endif 
