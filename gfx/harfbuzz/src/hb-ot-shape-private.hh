

























#ifndef HB_OT_SHAPE_PRIVATE_HH
#define HB_OT_SHAPE_PRIVATE_HH

#include "hb-private.hh"

#include "hb-ot-shape.h"

#include "hb-ot-map-private.hh"
#include "hb-ot-shape-complex-private.hh"



enum hb_ot_complex_shaper_t;

struct hb_ot_shape_plan_t
{
  friend struct hb_ot_shape_planner_t;

  hb_ot_map_t map;
  hb_ot_complex_shaper_t shaper;

  hb_ot_shape_plan_t (void) : map () {}
  ~hb_ot_shape_plan_t (void) { map.finish (); }

  private:
  NO_COPY (hb_ot_shape_plan_t);
};

struct hb_ot_shape_planner_t
{
  hb_ot_map_builder_t map;
  hb_ot_complex_shaper_t shaper;

  hb_ot_shape_planner_t (void) : map () {}
  ~hb_ot_shape_planner_t (void) { map.finish (); }

  inline void compile (hb_face_t *face,
		       const hb_segment_properties_t *props,
		       struct hb_ot_shape_plan_t &plan)
  {
    plan.shaper = shaper;
    map.compile (face, props, plan.map);
  }

  private:
  NO_COPY (hb_ot_shape_planner_t);
};


struct hb_ot_shape_context_t
{
  
  hb_ot_shape_plan_t *plan;
  hb_font_t *font;
  hb_face_t *face;
  hb_buffer_t  *buffer;
  const hb_feature_t *user_features;
  unsigned int        num_user_features;

  
  hb_direction_t target_direction;
  hb_bool_t applied_substitute_complex;
  hb_bool_t applied_position_complex;
};


static inline hb_bool_t
is_variation_selector (hb_codepoint_t unicode)
{
  return unlikely ((unicode >=  0x180B && unicode <=  0x180D) || 
		   (unicode >=  0xFE00 && unicode <=  0xFE0F) || 
		   (unicode >= 0xE0100 && unicode <= 0xE01EF));  
}

static inline unsigned int
_hb_unicode_modified_combining_class (hb_unicode_funcs_t *ufuncs,
				      hb_codepoint_t      unicode)
{
  int c = hb_unicode_combining_class (ufuncs, unicode);

  



  if (unlikely (hb_in_range<int> (c, 27, 33)))
    c = c == 33 ? 27 : c + 1;

  return c;
}

static inline void
hb_glyph_info_set_unicode_props (hb_glyph_info_t *info, hb_unicode_funcs_t *unicode)
{
  info->general_category() = hb_unicode_general_category (unicode, info->codepoint);
  info->combining_class() = _hb_unicode_modified_combining_class (unicode, info->codepoint);
}

HB_INTERNAL void _hb_set_unicode_props (hb_buffer_t *buffer);

HB_INTERNAL void _hb_ot_shape_normalize (hb_ot_shape_context_t *c);


#endif 
