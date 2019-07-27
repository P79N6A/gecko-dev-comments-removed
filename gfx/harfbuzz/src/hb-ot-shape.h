

























#ifndef HB_OT_H_IN
#error "Include <hb-ot.h> instead."
#endif

#ifndef HB_OT_SHAPE_H
#define HB_OT_SHAPE_H

#include "hb.h"

HB_BEGIN_DECLS


void
hb_ot_shape_glyphs_closure (hb_font_t          *font,
			    hb_buffer_t        *buffer,
			    const hb_feature_t *features,
			    unsigned int        num_features,
			    hb_set_t           *glyphs);

void
hb_ot_shape_plan_collect_lookups (hb_shape_plan_t *shape_plan,
				  hb_tag_t         table_tag,
				  hb_set_t        *lookup_indexes );

HB_END_DECLS

#endif 
