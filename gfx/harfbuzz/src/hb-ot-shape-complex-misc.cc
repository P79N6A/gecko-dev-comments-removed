

























#include "hb-ot-shape-complex-private.hh"











void
_hb_ot_shape_complex_collect_features_default (hb_ot_map_builder_t *map, const hb_segment_properties_t  *props)
{
}

hb_ot_shape_normalization_mode_t
_hb_ot_shape_complex_normalization_preference_default (void)
{
  return HB_OT_SHAPE_NORMALIZATION_MODE_COMPOSED_DIACRITICS;
}

void
_hb_ot_shape_complex_setup_masks_default (hb_ot_map_t *map, hb_buffer_t *buffer, hb_font_t *font)
{
}





static const hb_tag_t hangul_features[] =
{
  HB_TAG('l','j','m','o'),
  HB_TAG('v','j','m','o'),
  HB_TAG('t','j','m','o'),
};

void
_hb_ot_shape_complex_collect_features_hangul (hb_ot_map_builder_t *map, const hb_segment_properties_t  *props)
{
  for (unsigned int i = 0; i < ARRAY_LENGTH (hangul_features); i++)
    map->add_bool_feature (hangul_features[i]);
}

hb_ot_shape_normalization_mode_t
_hb_ot_shape_complex_normalization_preference_hangul (void)
{
  return HB_OT_SHAPE_NORMALIZATION_MODE_COMPOSED_FULL;
}

void
_hb_ot_shape_complex_setup_masks_hangul (hb_ot_map_t *map, hb_buffer_t *buffer, hb_font_t *font)
{
}





void
_hb_ot_shape_complex_collect_features_thai (hb_ot_map_builder_t *map, const hb_segment_properties_t  *props)
{
}

hb_ot_shape_normalization_mode_t
_hb_ot_shape_complex_normalization_preference_thai (void)
{
  return HB_OT_SHAPE_NORMALIZATION_MODE_COMPOSED_FULL;
}

void
_hb_ot_shape_complex_setup_masks_thai (hb_ot_map_t *map, hb_buffer_t *buffer, hb_font_t *font)
{
  















  














  

#define IS_SARA_AM(x) (((x) & ~0x0080) == 0x0E33)
#define NIKHAHIT_FROM_SARA_AM(x) ((x) - 0xE33 + 0xE4D)
#define SARA_AA_FROM_SARA_AM(x) ((x) - 1)
#define IS_TONE_MARK(x) (((x) & ~0x0083) == 0x0E48)

  buffer->clear_output ();
  unsigned int count = buffer->len;
  for (buffer->idx = 0; buffer->idx < count;)
  {
    hb_codepoint_t u = buffer->info[buffer->idx].codepoint;
    if (likely (!IS_SARA_AM (u))) {
      buffer->next_glyph ();
      continue;
    }

    
    uint16_t decomposed[2] = {uint16_t (NIKHAHIT_FROM_SARA_AM (u)),
			      uint16_t (SARA_AA_FROM_SARA_AM (u))};
    buffer->replace_glyphs (1, 2, decomposed);
    if (unlikely (buffer->in_error))
      return;

    
    unsigned int end = buffer->out_len;
    unsigned int start = end - 2;
    while (start > 0 && IS_TONE_MARK (buffer->out_info[start - 1].codepoint))
      start--;

    
    hb_glyph_info_t t = buffer->out_info[end - 2];
    memmove (buffer->out_info + start + 1,
	     buffer->out_info + start,
	     sizeof (buffer->out_info[0]) * (end - start - 2));
    buffer->out_info[start] = t;

    
    for (; start > 0 && buffer->out_info[start - 1].cluster == buffer->out_info[start].cluster; start--)
      ;
    for (; buffer->idx < count;)
      if (buffer->info[buffer->idx].cluster == buffer->out_info[buffer->out_len - 1].cluster)
        buffer->next_glyph ();
      else
        break;
    end = buffer->out_len;

    buffer->merge_out_clusters (start, end);
  }
  buffer->swap_buffers ();
}
