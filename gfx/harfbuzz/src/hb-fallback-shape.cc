

























#define HB_SHAPER fallback
#include "hb-shaper-impl-private.hh"






struct hb_fallback_shaper_face_data_t {};

hb_fallback_shaper_face_data_t *
_hb_fallback_shaper_face_data_create (hb_face_t *face)
{
  return (hb_fallback_shaper_face_data_t *) HB_SHAPER_DATA_SUCCEEDED;
}

void
_hb_fallback_shaper_face_data_destroy (hb_fallback_shaper_face_data_t *data)
{
}






struct hb_fallback_shaper_font_data_t {};

hb_fallback_shaper_font_data_t *
_hb_fallback_shaper_font_data_create (hb_font_t *font)
{
  return (hb_fallback_shaper_font_data_t *) HB_SHAPER_DATA_SUCCEEDED;
}

void
_hb_fallback_shaper_font_data_destroy (hb_fallback_shaper_font_data_t *data)
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
_hb_fallback_shape (hb_shape_plan_t    *shape_plan,
		    hb_font_t          *font,
		    hb_buffer_t        *buffer,
		    const hb_feature_t *features HB_UNUSED,
		    unsigned int        num_features HB_UNUSED)
{
  hb_codepoint_t space;
  font->get_glyph (' ', 0, &space);

  buffer->guess_segment_properties ();
  buffer->clear_positions ();

  unsigned int count = buffer->len;

  for (unsigned int i = 0; i < count; i++)
  {
    if (buffer->unicode->is_default_ignorable (buffer->info[i].codepoint)) {
      buffer->info[i].codepoint = space;
      buffer->pos[i].x_advance = 0;
      buffer->pos[i].y_advance = 0;
      continue;
    }
    font->get_glyph (buffer->info[i].codepoint, 0, &buffer->info[i].codepoint);
    font->get_glyph_advance_for_direction (buffer->info[i].codepoint,
					   buffer->props.direction,
					   &buffer->pos[i].x_advance,
					   &buffer->pos[i].y_advance);
    font->subtract_glyph_origin_for_direction (buffer->info[i].codepoint,
					       buffer->props.direction,
					       &buffer->pos[i].x_offset,
					       &buffer->pos[i].y_offset);
  }

  if (HB_DIRECTION_IS_BACKWARD (buffer->props.direction))
    hb_buffer_reverse (buffer);

  return true;
}
