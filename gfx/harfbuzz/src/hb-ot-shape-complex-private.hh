

























#ifndef HB_OT_SHAPE_COMPLEX_PRIVATE_HH
#define HB_OT_SHAPE_COMPLEX_PRIVATE_HH

#include "hb-private.h"

#include "hb-ot-shape-private.hh"

HB_BEGIN_DECLS


static inline hb_ot_complex_shaper_t
hb_ot_shape_complex_categorize (const hb_segment_properties_t *props)
{
  switch ((int) props->script) {
    case HB_SCRIPT_ARABIC:
    case HB_SCRIPT_NKO:
    case HB_SCRIPT_SYRIAC:
      return hb_ot_complex_shaper_arabic;

    default:
      return hb_ot_complex_shaper_none;
  }
}











HB_INTERNAL void _hb_ot_shape_complex_collect_features_arabic	(hb_ot_shape_plan_t *plan, const hb_segment_properties_t  *props);

static inline void
hb_ot_shape_complex_collect_features (hb_ot_shape_plan_t *plan,
				      const hb_segment_properties_t  *props)
{
  switch (plan->shaper) {
    case hb_ot_complex_shaper_arabic:	_hb_ot_shape_complex_collect_features_arabic (plan, props);	return;
    case hb_ot_complex_shaper_none:	default:							return;
  }
}









HB_INTERNAL void _hb_ot_shape_complex_setup_masks_arabic	(hb_ot_shape_context_t *c);

static inline void
hb_ot_shape_complex_setup_masks (hb_ot_shape_context_t *c)
{
  switch (c->plan->shaper) {
    case hb_ot_complex_shaper_arabic:	_hb_ot_shape_complex_setup_masks_arabic (c);	return;
    case hb_ot_complex_shaper_none:	default:					return;
  }
}


HB_END_DECLS

#endif 
