



























#define HB_SHAPER coretext
#include "hb-shaper-impl-private.hh"

#define GlyphID GlyphID_mac
#include <ApplicationServices/ApplicationServices.h>
#undef GlyphID

#include "hb-coretext.h"


#ifndef HB_DEBUG_CORETEXT
#define HB_DEBUG_CORETEXT (HB_DEBUG+0)
#endif


HB_SHAPER_DATA_ENSURE_DECLARE(coretext, face)
HB_SHAPER_DATA_ENSURE_DECLARE(coretext, font)






struct hb_coretext_shaper_face_data_t {
  CGFontRef cg_font;
};

static void
release_data (void *info, const void *data, size_t size)
{
  assert (hb_blob_get_length ((hb_blob_t *) info) == size &&
          hb_blob_get_data ((hb_blob_t *) info, NULL) == data);

  hb_blob_destroy ((hb_blob_t *) info);
}

hb_coretext_shaper_face_data_t *
_hb_coretext_shaper_face_data_create (hb_face_t *face)
{
  hb_coretext_shaper_face_data_t *data = (hb_coretext_shaper_face_data_t *) calloc (1, sizeof (hb_coretext_shaper_face_data_t));
  if (unlikely (!data))
    return NULL;

  hb_blob_t *blob = hb_face_reference_blob (face);
  unsigned int blob_length;
  const char *blob_data = hb_blob_get_data (blob, &blob_length);
  if (unlikely (!blob_length))
    DEBUG_MSG (CORETEXT, face, "Face has empty blob");

  CGDataProviderRef provider = CGDataProviderCreateWithData (blob, blob_data, blob_length, &release_data);
  data->cg_font = CGFontCreateWithDataProvider (provider);
  CGDataProviderRelease (provider);

  if (unlikely (!data->cg_font)) {
    DEBUG_MSG (CORETEXT, face, "Face CGFontCreateWithDataProvider() failed");
    free (data);
    return NULL;
  }

  return data;
}

void
_hb_coretext_shaper_face_data_destroy (hb_coretext_shaper_face_data_t *data)
{
  CFRelease (data->cg_font);
  free (data);
}






struct hb_coretext_shaper_font_data_t {
  CTFontRef ct_font;
};

hb_coretext_shaper_font_data_t *
_hb_coretext_shaper_font_data_create (hb_font_t *font)
{
  if (unlikely (!hb_coretext_shaper_face_data_ensure (font->face))) return NULL;

  hb_coretext_shaper_font_data_t *data = (hb_coretext_shaper_font_data_t *) calloc (1, sizeof (hb_coretext_shaper_font_data_t));
  if (unlikely (!data))
    return NULL;

  hb_face_t *face = font->face;
  hb_coretext_shaper_face_data_t *face_data = HB_SHAPER_DATA_GET (face);

  data->ct_font = CTFontCreateWithGraphicsFont (face_data->cg_font, font->y_scale, NULL, NULL);
  if (unlikely (!data->ct_font)) {
    DEBUG_MSG (CORETEXT, font, "Font CTFontCreateWithGraphicsFont() failed");
    free (data);
    return NULL;
  }

  return data;
}

void
_hb_coretext_shaper_font_data_destroy (hb_coretext_shaper_font_data_t *data)
{
  CFRelease (data->ct_font);
  free (data);
}






struct hb_coretext_shaper_shape_plan_data_t {};

hb_coretext_shaper_shape_plan_data_t *
_hb_coretext_shaper_shape_plan_data_create (hb_shape_plan_t    *shape_plan HB_UNUSED,
					     const hb_feature_t *user_features HB_UNUSED,
					     unsigned int        num_user_features HB_UNUSED)
{
  return (hb_coretext_shaper_shape_plan_data_t *) HB_SHAPER_DATA_SUCCEEDED;
}

void
_hb_coretext_shaper_shape_plan_data_destroy (hb_coretext_shaper_shape_plan_data_t *data HB_UNUSED)
{
}






CTFontRef
hb_coretext_font_get_ct_font (hb_font_t *font)
{
  if (unlikely (!hb_coretext_shaper_font_data_ensure (font))) return 0;
  hb_coretext_shaper_font_data_t *font_data = HB_SHAPER_DATA_GET (font);
  return font_data->ct_font;
}

hb_bool_t
_hb_coretext_shape (hb_shape_plan_t    *shape_plan,
		    hb_font_t          *font,
                    hb_buffer_t        *buffer,
                    const hb_feature_t *features,
                    unsigned int        num_features)
{
  hb_face_t *face = font->face;
  hb_coretext_shaper_face_data_t *face_data = HB_SHAPER_DATA_GET (face);
  hb_coretext_shaper_font_data_t *font_data = HB_SHAPER_DATA_GET (font);

#define FAIL(...) \
  HB_STMT_START { \
    DEBUG_MSG (CORETEXT, NULL, __VA_ARGS__); \
    return false; \
  } HB_STMT_END;

  unsigned int scratch_size;
  char *scratch = (char *) buffer->get_scratch_buffer (&scratch_size);

#define utf16_index() var1.u32

  UniChar *pchars = (UniChar *) scratch;
  unsigned int chars_len = 0;
  for (unsigned int i = 0; i < buffer->len; i++) {
    hb_codepoint_t c = buffer->info[i].codepoint;
    buffer->info[i].utf16_index() = chars_len;
    if (likely (c < 0x10000))
      pchars[chars_len++] = c;
    else if (unlikely (c >= 0x110000))
      pchars[chars_len++] = 0xFFFD;
    else {
      pchars[chars_len++] = 0xD800 + ((c - 0x10000) >> 10);
      pchars[chars_len++] = 0xDC00 + ((c - 0x10000) & ((1 << 10) - 1));
    }
  }

#undef utf16_index

  CFStringRef string_ref = CFStringCreateWithCharactersNoCopy (kCFAllocatorDefault,
                                                               pchars, chars_len,
                                                               kCFAllocatorNull);

  CFDictionaryRef attrs = CFDictionaryCreate (kCFAllocatorDefault,
                                              (const void**) &kCTFontAttributeName,
                                              (const void**) &font_data->ct_font,
                                              1, 
                                              &kCFTypeDictionaryKeyCallBacks,
                                              &kCFTypeDictionaryValueCallBacks);

  

  
  CFAttributedStringRef attr_string = CFAttributedStringCreate (kCFAllocatorDefault, string_ref, attrs);
  CFRelease (string_ref);
  CFRelease (attrs);

  
  CTLineRef line = CTLineCreateWithAttributedString (attr_string);
  CFRelease (attr_string);

  
  CFArrayRef glyph_runs = CTLineGetGlyphRuns (line);
  unsigned int num_runs = CFArrayGetCount (glyph_runs);

  
  bool success = true;
  buffer->len = 0;

  const CFRange range_all = CFRangeMake (0, 0);

  for (unsigned int i = 0; i < num_runs; i++) {
    CTRunRef run = (CTRunRef) CFArrayGetValueAtIndex (glyph_runs, i);

    unsigned int num_glyphs = CTRunGetGlyphCount (run);
    if (num_glyphs == 0)
      continue;

    buffer->ensure (buffer->len + num_glyphs);

    

    
    
    

    unsigned int scratch_size;
    char *scratch = (char *) buffer->get_scratch_buffer (&scratch_size);

#define ALLOCATE_ARRAY(Type, name, len) \
  Type *name = (Type *) scratch; \
  scratch += (len) * sizeof ((name)[0]); \
  scratch_size -= (len) * sizeof ((name)[0]);

    const CGGlyph* glyphs = CTRunGetGlyphsPtr (run);
    if (!glyphs) {
      ALLOCATE_ARRAY (CGGlyph, glyph_buf, num_glyphs);
      CTRunGetGlyphs (run, range_all, glyph_buf);
      glyphs = glyph_buf;
    }

    const CGPoint* positions = CTRunGetPositionsPtr (run);
    if (!positions) {
      ALLOCATE_ARRAY (CGPoint, position_buf, num_glyphs);
      CTRunGetPositions (run, range_all, position_buf);
      positions = position_buf;
    }

    const CFIndex* string_indices = CTRunGetStringIndicesPtr (run);
    if (!string_indices) {
      ALLOCATE_ARRAY (CFIndex, index_buf, num_glyphs);
      CTRunGetStringIndices (run, range_all, index_buf);
      string_indices = index_buf;
    }

#undef ALLOCATE_ARRAY

    double run_width = CTRunGetTypographicBounds (run, range_all, NULL, NULL, NULL);

    for (unsigned int j = 0; j < num_glyphs; j++) {
      double advance = (j + 1 < num_glyphs ? positions[j + 1].x : positions[0].x + run_width) - positions[j].x;

      hb_glyph_info_t *info = &buffer->info[buffer->len];
      hb_glyph_position_t *pos = &buffer->pos[buffer->len];

      info->codepoint = glyphs[j];
      info->cluster = string_indices[j];

      
      info->mask = advance;
      info->var1.u32 = 0;
      info->var2.u32 = positions[j].y;

      buffer->len++;
    }
  }

  buffer->clear_positions ();

  unsigned int count = buffer->len;
  for (unsigned int i = 0; i < count; ++i) {
    hb_glyph_info_t *info = &buffer->info[i];
    hb_glyph_position_t *pos = &buffer->pos[i];

    
    pos->x_advance = info->mask;
    pos->x_offset = info->var1.u32;
    pos->y_offset = info->var2.u32;
  }

  
  
  
  
  
  
  if (HB_DIRECTION_IS_FORWARD (buffer->props.direction)) {
    unsigned int prev_cluster = 0;
    for (unsigned int i = 0; i < count; i++) {
      unsigned int curr_cluster = buffer->info[i].cluster;
      if (curr_cluster < prev_cluster) {
        for (unsigned int j = i; j > 0; j--) {
          if (buffer->info[j - 1].cluster > curr_cluster)
            buffer->info[j - 1].cluster = curr_cluster;
          else
            break;
        }
      }
      prev_cluster = curr_cluster;
    }
  } else {
    
    unsigned int prev_cluster = (unsigned int)-1;
    for (unsigned int i = 0; i < count; i++) {
      unsigned int curr_cluster = buffer->info[i].cluster;
      if (curr_cluster > prev_cluster) {
        for (unsigned int j = i; j > 0; j--) {
          if (buffer->info[j - 1].cluster < curr_cluster)
            buffer->info[j - 1].cluster = curr_cluster;
          else
            break;
        }
      }
      prev_cluster = curr_cluster;
    }
  }

  return true;
}
