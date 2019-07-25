

























#ifndef HB_OT_SHAPE_PRIVATE_HH
#define HB_OT_SHAPE_PRIVATE_HH

#include "hb-private.hh"

#include "hb-ot-map-private.hh"
#include "hb-ot-shape-complex-private.hh"


struct hb_ot_shape_plan_t
{
  hb_ot_map_t map;
  hb_ot_complex_shaper_t shaper;

  hb_ot_shape_plan_t (void) : map () {}
  ~hb_ot_shape_plan_t (void) { map.finish (); }

  private:
  NO_COPY (hb_ot_shape_plan_t);
};



HB_INTERNAL hb_bool_t
_hb_ot_shape (hb_font_t          *font,
	      hb_buffer_t        *buffer,
	      const hb_feature_t *features,
	      unsigned int        num_features);

#endif 
