

























#ifndef HB_OT_SHAPE_PRIVATE_HH
#define HB_OT_SHAPE_PRIVATE_HH

#include "hb-private.h"

#include "hb-ot-shape.h"

#include "hb-ot-map-private.hh"

HB_BEGIN_DECLS



#define general_category() var1.u8[0] /* unicode general_category (hb_category_t) */
#define combining_class() var1.u8[1] /* unicode combining_class (uint8_t) */


enum hb_ot_complex_shaper_t {
  hb_ot_complex_shaper_none,
  hb_ot_complex_shaper_arabic
};


struct hb_ot_shape_plan_t
{
  hb_ot_map_t map;
  hb_ot_complex_shaper_t shaper;
};


struct hb_ot_shape_context_t
{
  
  hb_ot_shape_plan_t *plan;
  hb_font_t *font;
  hb_face_t *face;
  hb_buffer_t  *buffer;
  const hb_feature_t *user_features;
  unsigned int        num_user_features;

  
  hb_direction_t original_direction;
  hb_bool_t applied_substitute_complex;
  hb_bool_t applied_position_complex;
};


HB_END_DECLS

#endif 
