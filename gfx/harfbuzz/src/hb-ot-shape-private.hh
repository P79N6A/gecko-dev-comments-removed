

























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


inline void
_hb_glyph_info_set_unicode_props (hb_glyph_info_t *info, hb_unicode_funcs_t *unicode)
{
  info->unicode_props0() = ((unsigned int) hb_unicode_general_category (unicode, info->codepoint)) |
			   (_hb_unicode_is_zero_width (info->codepoint) ? 0x80 : 0);
  info->unicode_props1() = _hb_unicode_modified_combining_class (unicode, info->codepoint);
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
