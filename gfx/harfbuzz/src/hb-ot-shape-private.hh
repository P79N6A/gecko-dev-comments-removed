

























#ifndef HB_OT_SHAPE_PRIVATE_HH
#define HB_OT_SHAPE_PRIVATE_HH

#include "hb-private.hh"

#include "hb-ot-map-private.hh"




#define unicode_props0()	var2.u8[0]
#define unicode_props1()	var2.u8[1]



struct hb_ot_shape_plan_t
{
  hb_segment_properties_t props;
  const struct hb_ot_complex_shaper_t *shaper;
  hb_ot_map_t map;
  const void *data;

  inline void substitute_closure (hb_face_t *face, hb_set_t *glyphs) const { map.substitute_closure (this, face, glyphs); }
  inline void substitute (hb_font_t *font, hb_buffer_t *buffer) const { map.substitute (this, font, buffer); }
  inline void position (hb_font_t *font, hb_buffer_t *buffer) const { map.position (this, font, buffer); }

  void finish (void) { map.finish (); }
};

struct hb_ot_shape_planner_t
{
  
  hb_face_t *face;
  hb_segment_properties_t props;
  const struct hb_ot_complex_shaper_t *shaper;
  hb_ot_map_builder_t map;

  hb_ot_shape_planner_t (const hb_shape_plan_t *master_plan) :
			 face (master_plan->face),
			 props (master_plan->props),
			 shaper (NULL),
			 map () {}
  ~hb_ot_shape_planner_t (void) { map.finish (); }

  inline void compile (hb_ot_shape_plan_t &plan)
  {
    plan.props = props;
    plan.shaper = shaper;
    map.compile (face, &props, plan.map);
  }

  private:
  NO_COPY (hb_ot_shape_planner_t);
};



inline void
_hb_glyph_info_set_unicode_props (hb_glyph_info_t *info, hb_unicode_funcs_t *unicode)
{
  info->unicode_props0() = ((unsigned int) unicode->general_category (info->codepoint)) |
			   (unicode->is_zero_width (info->codepoint) ? 0x80 : 0);
  info->unicode_props1() = unicode->modified_combining_class (info->codepoint);
}

inline hb_unicode_general_category_t
_hb_glyph_info_get_general_category (const hb_glyph_info_t *info)
{
  return (hb_unicode_general_category_t) (info->unicode_props0() & 0x7F);
}

inline unsigned int
_hb_glyph_info_get_modified_combining_class (const hb_glyph_info_t *info)
{
  return info->unicode_props1();
}

inline hb_bool_t
_hb_glyph_info_is_zero_width (const hb_glyph_info_t *info)
{
  return !!(info->unicode_props0() & 0x80);
}

#endif 
