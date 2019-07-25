

























#include "hb-ot-shape-complex-private.hh"







static const hb_tag_t hangul_features[] =
{
  HB_TAG('l','j','m','o'),
  HB_TAG('v','j','m','o'),
  HB_TAG('t','j','m','o'),
  HB_TAG_NONE
};

static const hb_tag_t tibetan_features[] =
{
  HB_TAG('a','b','v','s'),
  HB_TAG('b','l','w','s'),
  HB_TAG('a','b','v','m'),
  HB_TAG('b','l','w','m'),
  HB_TAG_NONE
};

static void
collect_features_default (hb_ot_shape_planner_t *plan)
{
  const hb_tag_t *script_features = NULL;

  switch ((hb_tag_t) plan->props.script)
  {
    
    case HB_SCRIPT_HANGUL:
      script_features = hangul_features;
      break;

    
    case HB_SCRIPT_TIBETAN:
      script_features = tibetan_features;
      break;
  }

  for (; script_features && *script_features; script_features++)
    plan->map.add_bool_feature (*script_features);
}

static hb_ot_shape_normalization_mode_t
normalization_preference_default (const hb_ot_shape_plan_t *plan)
{
  switch ((hb_tag_t) plan->props.script)
  {
    
    case HB_SCRIPT_HANGUL:
      return HB_OT_SHAPE_NORMALIZATION_MODE_COMPOSED_FULL;
  }
  return HB_OT_SHAPE_NORMALIZATION_MODE_COMPOSED_DIACRITICS;
}

const hb_ot_complex_shaper_t _hb_ot_complex_shaper_default =
{
  "default",
  collect_features_default,
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  normalization_preference_default,
  NULL, 
  true, 
};




static void
preprocess_text_thai (const hb_ot_shape_plan_t *plan HB_UNUSED,
		      hb_buffer_t              *buffer,
		      hb_font_t                *font HB_UNUSED)
{
  























  














  

#define IS_SARA_AM(x) (((x) & ~0x0080) == 0x0E33)
#define NIKHAHIT_FROM_SARA_AM(x) ((x) - 0xE33 + 0xE4D)
#define SARA_AA_FROM_SARA_AM(x) ((x) - 1)
#define IS_TONE_MARK(x) (hb_in_ranges<hb_codepoint_t> ((x) & ~0x0080, 0x0E34, 0x0E37, 0x0E47, 0x0E4E, 0x0E31, 0x0E31))

  buffer->clear_output ();
  unsigned int count = buffer->len;
  for (buffer->idx = 0; buffer->idx < count;)
  {
    hb_codepoint_t u = buffer->cur().codepoint;
    if (likely (!IS_SARA_AM (u))) {
      buffer->next_glyph ();
      continue;
    }

    
    hb_codepoint_t decomposed[2] = {hb_codepoint_t (NIKHAHIT_FROM_SARA_AM (u)),
				    hb_codepoint_t (SARA_AA_FROM_SARA_AM (u))};
    buffer->replace_glyphs (1, 2, decomposed);
    if (unlikely (buffer->in_error))
      return;

    
    unsigned int end = buffer->out_len;
    unsigned int start = end - 2;
    while (start > 0 && IS_TONE_MARK (buffer->out_info[start - 1].codepoint))
      start--;

    if (start + 2 < end)
    {
      
      buffer->merge_out_clusters (start, end);
      hb_glyph_info_t t = buffer->out_info[end - 2];
      memmove (buffer->out_info + start + 1,
	       buffer->out_info + start,
	       sizeof (buffer->out_info[0]) * (end - start - 2));
      buffer->out_info[start] = t;
    }
    else
    {
      

      if (start)
	buffer->merge_out_clusters (start - 1, end);
    }
  }
  buffer->swap_buffers ();
}

const hb_ot_complex_shaper_t _hb_ot_complex_shaper_thai =
{
  "thai",
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  preprocess_text_thai,
  NULL, 
  NULL, 
  true, 
};
