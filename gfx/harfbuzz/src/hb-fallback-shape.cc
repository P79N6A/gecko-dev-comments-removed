

























#include "hb-fallback-shape-private.hh"

#include "hb-buffer-private.hh"

hb_bool_t
hb_fallback_shape (hb_font_t          *font,
		   hb_buffer_t        *buffer,
		   const hb_feature_t *features,
		   unsigned int        num_features,
		   const char * const *shaper_options)
{
  buffer->guess_properties ();

  unsigned int count = buffer->len;

  for (unsigned int i = 0; i < count; i++)
    hb_font_get_glyph (font, buffer->info[i].codepoint, 0, &buffer->info[i].codepoint);

  buffer->clear_positions ();

  for (unsigned int i = 0; i < count; i++) {
    hb_font_get_glyph_advance_for_direction (font, buffer->info[i].codepoint,
					     buffer->props.direction,
					     &buffer->pos[i].x_advance,
					     &buffer->pos[i].y_advance);
    hb_font_subtract_glyph_origin_for_direction (font, buffer->info[i].codepoint,
						 buffer->props.direction,
						 &buffer->pos[i].x_offset,
						 &buffer->pos[i].y_offset);
  }

  if (HB_DIRECTION_IS_BACKWARD (buffer->props.direction))
    hb_buffer_reverse (buffer);

  return TRUE;
}
