

























#include "hb-ot-shape-complex-private.hh"






enum {
  NONE,

  LJMO,
  VJMO,
  TJMO,

  FIRST_HANGUL_FEATURE = LJMO,
  HANGUL_FEATURE_COUNT = TJMO + 1
};

static const hb_tag_t hangul_features[HANGUL_FEATURE_COUNT] =
{
  HB_TAG_NONE,
  HB_TAG('l','j','m','o'),
  HB_TAG('v','j','m','o'),
  HB_TAG('t','j','m','o')
};

static void
collect_features_hangul (hb_ot_shape_planner_t *plan)
{
  hb_ot_map_builder_t *map = &plan->map;

  for (unsigned int i = FIRST_HANGUL_FEATURE; i < HANGUL_FEATURE_COUNT; i++)
    map->add_feature (hangul_features[i], 1, F_NONE);
}

static void
override_features_hangul (hb_ot_shape_planner_t *plan)
{
  


  plan->map.add_feature (HB_TAG('c','a','l','t'), 0, F_GLOBAL);
}

struct hangul_shape_plan_t
{
  ASSERT_POD ();

  hb_mask_t mask_array[HANGUL_FEATURE_COUNT];
};

static void *
data_create_hangul (const hb_ot_shape_plan_t *plan)
{
  hangul_shape_plan_t *hangul_plan = (hangul_shape_plan_t *) calloc (1, sizeof (hangul_shape_plan_t));
  if (unlikely (!hangul_plan))
    return NULL;

  for (unsigned int i = 0; i < HANGUL_FEATURE_COUNT; i++)
    hangul_plan->mask_array[i] = plan->map.get_1_mask (hangul_features[i]);

  return hangul_plan;
}

static void
data_destroy_hangul (void *data)
{
  free (data);
}


#define LBase 0x1100u
#define VBase 0x1161u
#define TBase 0x11A7u
#define LCount 19u
#define VCount 21u
#define TCount 28u
#define SBase 0xAC00u
#define NCount (VCount * TCount)
#define SCount (LCount * NCount)

#define isCombiningL(u) (hb_in_range ((u), LBase, LBase+LCount-1))
#define isCombiningV(u) (hb_in_range ((u), VBase, VBase+VCount-1))
#define isCombiningT(u) (hb_in_range ((u), TBase+1, TBase+TCount-1))
#define isCombinedS(u) (hb_in_range ((u), SBase, SBase+SCount-1))

#define isL(u) (hb_in_ranges ((u), 0x1100u, 0x115Fu, 0xA960u, 0xA97Cu))
#define isV(u) (hb_in_ranges ((u), 0x1160u, 0x11A7u, 0xD7B0u, 0xD7C6u))
#define isT(u) (hb_in_ranges ((u), 0x11A8u, 0x11FFu, 0xD7CBu, 0xD7FBu))

#define isHangulTone(u) (hb_in_range ((u), 0x302Eu, 0x302Fu))


#define hangul_shaping_feature() complex_var_u8_0() /* hangul jamo shaping feature */

static bool
is_zero_width_char (hb_font_t *font,
		    hb_codepoint_t unicode)
{
  hb_codepoint_t glyph;
  return hb_font_get_glyph (font, unicode, 0, &glyph) && hb_font_get_glyph_h_advance (font, glyph) == 0;
}

static void
preprocess_text_hangul (const hb_ot_shape_plan_t *plan,
			hb_buffer_t              *buffer,
			hb_font_t                *font)
{
  HB_BUFFER_ALLOCATE_VAR (buffer, hangul_shaping_feature);

  















































  buffer->clear_output ();
  unsigned int start = 0, end = 0; 


  unsigned int count = buffer->len;

  for (buffer->idx = 0; buffer->idx < count;)
  {
    hb_codepoint_t u = buffer->cur().codepoint;

    if (isHangulTone (u))
    {
      




      if (start < end && end == buffer->out_len)
      {
	
	buffer->next_glyph ();
	if (!is_zero_width_char (font, u))
	{
	  hb_glyph_info_t *info = buffer->out_info;
	  hb_glyph_info_t tone = info[end];
	  memmove (&info[start + 1], &info[start], (end - start) * sizeof (hb_glyph_info_t));
	  info[start] = tone;
	}
	




	buffer->merge_out_clusters (start, end + 1);
      }
      else
      {
	
	if (font->has_glyph (0x25CCu))
	{
	  hb_codepoint_t chars[2];
	  if (!is_zero_width_char (font, u)) {
	    chars[0] = u;
	    chars[1] = 0x25CCu;
	  } else {
	    chars[0] = 0x25CCu;
	    chars[1] = u;
	  }
	  buffer->replace_glyphs (1, 2, chars);
	}
	else
	{
	  
	  buffer->next_glyph ();
	}
      }
      start = end = buffer->out_len;
      continue;
    }

    start = buffer->out_len; 



    if (isL (u) && buffer->idx + 1 < count)
    {
      hb_codepoint_t l = u;
      hb_codepoint_t v = buffer->cur(+1).codepoint;
      if (isV (v))
      {
	
	hb_codepoint_t t = 0;
	unsigned int tindex = 0;
	if (buffer->idx + 2 < count)
	{
	  t = buffer->cur(+2).codepoint;
	  if (isT (t))
	    tindex = t - TBase; 
	  else
	    t = 0; 
	}

	
	if (isCombiningL (l) && isCombiningV (v) && (t == 0 || isCombiningT (t)))
	{
	  
	  hb_codepoint_t s = SBase + (l - LBase) * NCount + (v - VBase) * TCount + tindex;
	  if (font->has_glyph (s))
	  {
	    buffer->replace_glyphs (t ? 3 : 2, 1, &s);
	    if (unlikely (buffer->in_error))
	      return;
	    end = start + 1;
	    continue;
	  }
	}

	




	buffer->cur().hangul_shaping_feature() = LJMO;
	buffer->next_glyph ();
	buffer->cur().hangul_shaping_feature() = VJMO;
	buffer->next_glyph ();
	if (t)
	{
	  buffer->cur().hangul_shaping_feature() = TJMO;
	  buffer->next_glyph ();
	  end = start + 3;
	}
	else
	  end = start + 2;
	buffer->merge_out_clusters (start, end);
	continue;
      }
    }

    else if (isCombinedS (u))
    {
      
      hb_codepoint_t s = u;
      bool has_glyph = font->has_glyph (s);
      unsigned int lindex = (s - SBase) / NCount;
      unsigned int nindex = (s - SBase) % NCount;
      unsigned int vindex = nindex / TCount;
      unsigned int tindex = nindex % TCount;

      if (!tindex &&
	  buffer->idx + 1 < count &&
	  isCombiningT (buffer->cur(+1).codepoint))
      {
	
	unsigned int new_tindex = buffer->cur(+1).codepoint - TBase;
	hb_codepoint_t new_s = s + new_tindex;
	if (font->has_glyph (new_s))
	{
	  buffer->replace_glyphs (2, 1, &new_s);
	  if (unlikely (buffer->in_error))
	    return;
	  end = start + 1;
	  continue;
	}
      }

      


      if (!has_glyph ||
	  (!tindex &&
	   buffer->idx + 1 < count &&
	   isT (buffer->cur(+1).codepoint)))
      {
	hb_codepoint_t decomposed[3] = {LBase + lindex,
					VBase + vindex,
					TBase + tindex};
	if (font->has_glyph (decomposed[0]) &&
	    font->has_glyph (decomposed[1]) &&
	    (!tindex || font->has_glyph (decomposed[2])))
	{
	  unsigned int s_len = tindex ? 3 : 2;
	  buffer->replace_glyphs (1, s_len, decomposed);
	  if (unlikely (buffer->in_error))
	    return;

	  


	  hb_glyph_info_t *info = buffer->out_info;

	  


	  if (has_glyph && !tindex)
	  {
            buffer->next_glyph ();
            s_len++;
          }
          end = start + s_len;

	  unsigned int i = start;
	  info[i++].hangul_shaping_feature() = LJMO;
	  info[i++].hangul_shaping_feature() = VJMO;
	  if (i < end)
	    info[i++].hangul_shaping_feature() = TJMO;
	  buffer->merge_out_clusters (start, end);
	  continue;
	}
      }

      if (has_glyph)
      {
        
	end = start + 1;
	buffer->next_glyph ();
	continue;
      }
    }

    


    buffer->next_glyph ();
  }
  buffer->swap_buffers ();
}

static void
setup_masks_hangul (const hb_ot_shape_plan_t *plan,
		    hb_buffer_t              *buffer,
		    hb_font_t                *font HB_UNUSED)
{
  const hangul_shape_plan_t *hangul_plan = (const hangul_shape_plan_t *) plan->data;

  if (likely (hangul_plan))
  {
    unsigned int count = buffer->len;
    hb_glyph_info_t *info = buffer->info;
    for (unsigned int i = 0; i < count; i++, info++)
      info->mask |= hangul_plan->mask_array[info->hangul_shaping_feature()];
  }

  HB_BUFFER_DEALLOCATE_VAR (buffer, hangul_shaping_feature);
}


const hb_ot_complex_shaper_t _hb_ot_complex_shaper_hangul =
{
  "hangul",
  collect_features_hangul,
  override_features_hangul,
  data_create_hangul, 
  data_destroy_hangul, 
  preprocess_text_hangul,
  HB_OT_SHAPE_NORMALIZATION_MODE_NONE,
  NULL, 
  NULL, 
  setup_masks_hangul, 
  HB_OT_SHAPE_ZERO_WIDTH_MARKS_NONE,
  false, 
};
