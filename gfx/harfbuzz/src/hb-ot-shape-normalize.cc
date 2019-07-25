

























#include "hb-ot-shape-normalize-private.hh"
#include "hb-ot-shape-private.hh"










































static inline void
set_unicode_props (hb_glyph_info_t *info, hb_unicode_funcs_t *unicode)
{
  info->general_category() = hb_unicode_general_category (unicode, info->codepoint);
  info->combining_class() = _hb_unicode_modified_combining_class (unicode, info->codepoint);
}

static void
output_glyph (hb_font_t *font, hb_buffer_t *buffer,
	      hb_codepoint_t glyph)
{
  buffer->output_glyph (glyph);
  set_unicode_props (&buffer->out_info[buffer->out_len - 1], buffer->unicode);
}

static bool
decompose (hb_font_t *font, hb_buffer_t *buffer,
	   bool shortest,
	   hb_codepoint_t ab)
{
  hb_codepoint_t a, b, glyph;

  if (!hb_unicode_decompose (buffer->unicode, ab, &a, &b) ||
      (b && !hb_font_get_glyph (font, b, 0, &glyph)))
    return FALSE;

  bool has_a = hb_font_get_glyph (font, a, 0, &glyph);
  if (shortest && has_a) {
    
    output_glyph (font, buffer, a);
    if (b)
      output_glyph (font, buffer, b);
    return TRUE;
  }

  if (decompose (font, buffer, shortest, a)) {
    if (b)
      output_glyph (font, buffer, b);
    return TRUE;
  }

  if (has_a) {
    output_glyph (font, buffer, a);
    if (b)
      output_glyph (font, buffer, b);
    return TRUE;
  }

  return FALSE;
}

static void
decompose_current_glyph (hb_font_t *font, hb_buffer_t *buffer,
			 bool shortest)
{
  if (decompose (font, buffer, shortest, buffer->info[buffer->idx].codepoint))
    buffer->skip_glyph ();
  else
    buffer->next_glyph ();
}

static void
decompose_single_char_cluster (hb_font_t *font, hb_buffer_t *buffer,
			       bool will_recompose)
{
  hb_codepoint_t glyph;

  
  if (will_recompose && hb_font_get_glyph (font, buffer->info[buffer->idx].codepoint, 0, &glyph)) {
    buffer->next_glyph ();
    return;
  }

  decompose_current_glyph (font, buffer, will_recompose);
}

static void
decompose_multi_char_cluster (hb_font_t *font, hb_buffer_t *buffer,
			      unsigned int end)
{
  
  for (unsigned int i = buffer->idx; i < end; i++)
    if (unlikely (_hb_unicode_is_variation_selector (buffer->info[i].codepoint))) {
      while (buffer->idx < end)
	buffer->next_glyph ();
      return;
    }

  while (buffer->idx < end)
    decompose_current_glyph (font, buffer, FALSE);
}

static int
compare_combining_class (const hb_glyph_info_t *pa, const hb_glyph_info_t *pb)
{
  unsigned int a = pa->combining_class();
  unsigned int b = pb->combining_class();

  return a < b ? -1 : a == b ? 0 : +1;
}

void
_hb_ot_shape_normalize (hb_font_t *font, hb_buffer_t *buffer,
			hb_ot_shape_normalization_mode_t mode)
{
  bool recompose = mode != HB_OT_SHAPE_NORMALIZATION_MODE_DECOMPOSED;
  bool has_multichar_clusters = FALSE;
  unsigned int count;

  






  

  buffer->clear_output ();
  count = buffer->len;
  for (buffer->idx = 0; buffer->idx < count;)
  {
    unsigned int end;
    for (end = buffer->idx + 1; end < count; end++)
      if (buffer->info[buffer->idx].cluster != buffer->info[end].cluster)
        break;

    if (buffer->idx + 1 == end)
      decompose_single_char_cluster (font, buffer, recompose);
    else {
      decompose_multi_char_cluster (font, buffer, end);
      has_multichar_clusters = TRUE;
    }
  }
  buffer->swap_buffers ();


  if (mode != HB_OT_SHAPE_NORMALIZATION_MODE_COMPOSED_FULL && !has_multichar_clusters)
    return; 


  

  count = buffer->len;
  for (unsigned int i = 0; i < count; i++)
  {
    if (buffer->info[i].combining_class() == 0)
      continue;

    unsigned int end;
    for (end = i + 1; end < count; end++)
      if (buffer->info[end].combining_class() == 0)
        break;

    


    if (end - i > 10) {
      i = end;
      continue;
    }

    hb_bubble_sort (buffer->info + i, end - i, compare_combining_class);

    i = end;
  }


  if (!recompose)
    return;

  

  


  buffer->clear_output ();
  count = buffer->len;
  unsigned int starter = 0;
  buffer->next_glyph ();
  while (buffer->idx < count)
  {
    hb_codepoint_t composed, glyph;
    if (

	(mode == HB_OT_SHAPE_NORMALIZATION_MODE_COMPOSED_FULL ||
	 buffer->info[buffer->idx].combining_class() != 0) &&
	

	(starter == buffer->out_len - 1 ||
	 buffer->out_info[buffer->out_len - 1].combining_class() < buffer->info[buffer->idx].combining_class()) &&
	
	hb_unicode_compose (buffer->unicode,
			    buffer->out_info[starter].codepoint,
			    buffer->info[buffer->idx].codepoint,
			    &composed) &&
	
	hb_font_get_glyph (font, composed, 0, &glyph))
    {
      
      buffer->out_info[starter].codepoint = composed;
      set_unicode_props (&buffer->out_info[starter], buffer->unicode);

      buffer->skip_glyph ();
      continue;
    }

    
    buffer->next_glyph ();

    if (buffer->out_info[buffer->out_len - 1].combining_class() == 0)
      starter = buffer->out_len - 1;
  }
  buffer->swap_buffers ();

}
