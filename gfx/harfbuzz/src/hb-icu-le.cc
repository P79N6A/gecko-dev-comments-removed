

























#define HB_SHAPER icu_le
#define hb_icu_le_shaper_font_data_t PortableFontInstance
#include "hb-shaper-impl-private.hh"

#include "hb-icu-le/PortableFontInstance.h"

#include "layout/loengine.h"
#include "unicode/unistr.h"

#include "hb-icu.h"






struct hb_icu_le_shaper_face_data_t {};

hb_icu_le_shaper_face_data_t *
_hb_icu_le_shaper_face_data_create (hb_face_t *face HB_UNUSED)
{
  return (hb_icu_le_shaper_face_data_t *) HB_SHAPER_DATA_SUCCEEDED;
}

void
_hb_icu_le_shaper_face_data_destroy (hb_icu_le_shaper_face_data_t *data HB_UNUSED)
{
}






hb_icu_le_shaper_font_data_t *
_hb_icu_le_shaper_font_data_create (hb_font_t *font)
{
  LEErrorCode status = LE_NO_ERROR;
  hb_icu_le_shaper_font_data_t *data = new PortableFontInstance (font->face,
								 font->x_scale,
								 font->y_scale,
								 status);
  if (status != LE_NO_ERROR) {
    delete (data);
    return NULL;
  }

  return data;
}

void
_hb_icu_le_shaper_font_data_destroy (hb_icu_le_shaper_font_data_t *data)
{
  delete (data);
}






struct hb_icu_le_shaper_shape_plan_data_t {};

hb_icu_le_shaper_shape_plan_data_t *
_hb_icu_le_shaper_shape_plan_data_create (hb_shape_plan_t    *shape_plan HB_UNUSED,
					  const hb_feature_t *user_features,
					  unsigned int        num_user_features)
{
  return (hb_icu_le_shaper_shape_plan_data_t *) HB_SHAPER_DATA_SUCCEEDED;
}

void
_hb_icu_le_shaper_shape_plan_data_destroy (hb_icu_le_shaper_shape_plan_data_t *data)
{
}






hb_bool_t
_hb_icu_le_shape (hb_shape_plan_t    *shape_plan,
		  hb_font_t          *font,
		  hb_buffer_t        *buffer,
		  const hb_feature_t *features,
		  unsigned int        num_features)
{
  LEFontInstance *font_instance = HB_SHAPER_DATA_GET (font);
  le_int32 script_code = hb_icu_script_from_script (shape_plan->props.script);
  le_int32 language_code = -1 ;
  le_int32 typography_flags = 3; 
  LEErrorCode status = LE_NO_ERROR;
  le_engine *le = le_create ((const le_font *) font_instance,
			     script_code,
			     language_code,
			     typography_flags,
			     &status);
  if (status != LE_NO_ERROR)
  { le_close (le); return false; }

retry:

  unsigned int scratch_size;
  char *scratch = (char *) buffer->get_scratch_buffer (&scratch_size);

#define ALLOCATE_ARRAY(Type, name, len) \
  Type *name = (Type *) scratch; \
  scratch += (len) * sizeof ((name)[0]); \
  scratch_size -= (len) * sizeof ((name)[0]);

  ALLOCATE_ARRAY (LEUnicode, chars, buffer->len);
  ALLOCATE_ARRAY (unsigned int, clusters, buffer->len);

  
  for (unsigned int i = 0; i < buffer->len; i++) {
    chars[i] = buffer->info[i].codepoint;
    clusters[i] = buffer->info[i].cluster;
  }

  unsigned int glyph_count = le_layoutChars (le,
					     chars,
					     0,
					     buffer->len,
					     buffer->len,
					     HB_DIRECTION_IS_BACKWARD (buffer->props.direction),
					     0., 0.,
					     &status);
  if (status != LE_NO_ERROR)
  { le_close (le); return false; }

  unsigned int num_glyphs = scratch_size / (sizeof (LEGlyphID) +
					    sizeof (le_int32) +
					    sizeof (float) * 2);

  if (unlikely (glyph_count >= num_glyphs || glyph_count > buffer->allocated)) {
    buffer->ensure (buffer->allocated * 2);
    if (buffer->in_error)
    { le_close (le); return false; }
    goto retry;
  }

  ALLOCATE_ARRAY (LEGlyphID, glyphs, glyph_count);
  ALLOCATE_ARRAY (le_int32, indices, glyph_count);
  ALLOCATE_ARRAY (float, positions, glyph_count * 2 + 2);

  le_getGlyphs (le, glyphs, &status);
  le_getCharIndices (le, indices, &status);
  le_getGlyphPositions (le, positions, &status);

#undef ALLOCATE_ARRAY

  


  unsigned int j = 0;
  hb_glyph_info_t *info = buffer->info;
  for (unsigned int i = 0; i < glyph_count; i++)
  {
    if (glyphs[i] >= 0xFFFE)
	continue;

    info[j].codepoint = glyphs[i];
    info[j].cluster = clusters[indices[i]];

    
    info[j].mask = positions[2 * i + 2] - positions[2 * i];
    info[j].var1.u32 = 0;
    info[j].var2.u32 = -positions[2 * i + 1];

    j++;
  }
  buffer->len = j;

  buffer->clear_positions ();

  for (unsigned int i = 0; i < buffer->len; i++) {
    hb_glyph_info_t *info = &buffer->info[i];
    hb_glyph_position_t *pos = &buffer->pos[i];

    
    pos->x_advance = info->mask;
    pos->x_offset = info->var1.u32;
    pos->y_offset = info->var2.u32;
  }

  le_close (le);
  return true;
}
