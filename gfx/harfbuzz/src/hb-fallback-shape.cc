

























#define HB_SHAPER fallback
#include "hb-shaper-impl-private.hh"






struct hb_fallback_shaper_face_data_t {};

hb_fallback_shaper_face_data_t *
_hb_fallback_shaper_face_data_create (hb_face_t *face HB_UNUSED)
{
  return (hb_fallback_shaper_face_data_t *) HB_SHAPER_DATA_SUCCEEDED;
}

void
_hb_fallback_shaper_face_data_destroy (hb_fallback_shaper_face_data_t *data HB_UNUSED)
{
}






struct hb_fallback_shaper_font_data_t {};

hb_fallback_shaper_font_data_t *
_hb_fallback_shaper_font_data_create (hb_font_t *font HB_UNUSED)
{
  return (hb_fallback_shaper_font_data_t *) HB_SHAPER_DATA_SUCCEEDED;
}

void
_hb_fallback_shaper_font_data_destroy (hb_fallback_shaper_font_data_t *data HB_UNUSED)
{
}






struct hb_fallback_shaper_shape_plan_data_t {};

hb_fallback_shaper_shape_plan_data_t *
_hb_fallback_shaper_shape_plan_data_create (hb_shape_plan_t    *shape_plan HB_UNUSED,
					    const hb_feature_t *user_features HB_UNUSED,
					    unsigned int        num_user_features HB_UNUSED)
{
  return (hb_fallback_shaper_shape_plan_data_t *) HB_SHAPER_DATA_SUCCEEDED;
}

void
_hb_fallback_shaper_shape_plan_data_destroy (hb_fallback_shaper_shape_plan_data_t *data HB_UNUSED)
{
}






hb_bool_t
_hb_fallback_shape (hb_shape_plan_t    *shape_plan HB_UNUSED,
		    hb_font_t          *font,
		    hb_buffer_t        *buffer,
		    const hb_feature_t *features HB_UNUSED,
		    unsigned int        num_features HB_UNUSED)
{
  









  hb_codepoint_t space;
  bool has_space = font->get_glyph (' ', 0, &space);

  buffer->clear_positions ();

  hb_direction_t direction = buffer->props.direction;
  hb_unicode_funcs_t *unicode = buffer->unicode;
  unsigned int count = buffer->len;
  hb_glyph_info_t *info = buffer->info;
  hb_glyph_position_t *pos = buffer->pos;
  for (unsigned int i = 0; i < count; i++)
  {
    if (has_space && unicode->is_default_ignorable (info[i].codepoint)) {
      info[i].codepoint = space;
      pos[i].x_advance = 0;
      pos[i].y_advance = 0;
      continue;
    }
    font->get_glyph (info[i].codepoint, 0, &info[i].codepoint);
    font->get_glyph_advance_for_direction (info[i].codepoint,
					   direction,
					   &pos[i].x_advance,
					   &pos[i].y_advance);
    font->subtract_glyph_origin_for_direction (info[i].codepoint,
					       direction,
					       &pos[i].x_offset,
					       &pos[i].y_offset);
  }

  if (HB_DIRECTION_IS_BACKWARD (direction))
    hb_buffer_reverse (buffer);

  return true;
}
