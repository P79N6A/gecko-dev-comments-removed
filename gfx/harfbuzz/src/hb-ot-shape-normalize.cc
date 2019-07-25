

























#include "hb-ot-shape-normalize-private.hh"
#include "hb-ot-shape-private.hh"























































static void
output_glyph (hb_buffer_t *buffer, hb_codepoint_t glyph)
{
  buffer->output_glyph (glyph);
  _hb_glyph_info_set_unicode_props (&buffer->prev(), buffer->unicode);
}

static bool
decompose (hb_font_t *font, hb_buffer_t *buffer,
	   bool shortest,
	   hb_codepoint_t ab)
{
  hb_codepoint_t a, b, glyph;

  if (!buffer->unicode->decompose (ab, &a, &b) ||
      (b && !font->get_glyph (b, 0, &glyph)))
    return false;

  bool has_a = font->get_glyph (a, 0, &glyph);
  if (shortest && has_a) {
    
    output_glyph (buffer, a);
    if (b)
      output_glyph (buffer, b);
    return true;
  }

  if (decompose (font, buffer, shortest, a)) {
    if (b)
      output_glyph (buffer, b);
    return true;
  }

  if (has_a) {
    output_glyph (buffer, a);
    if (b)
      output_glyph (buffer, b);
    return true;
  }

  return false;
}

static bool
decompose_compatibility (hb_font_t *font, hb_buffer_t *buffer,
			 hb_codepoint_t u)
{
  unsigned int len, i;
  hb_codepoint_t decomposed[HB_UNICODE_MAX_DECOMPOSITION_LEN];

  len = buffer->unicode->decompose_compatibility (u, decomposed);
  if (!len)
    return false;

  hb_codepoint_t glyph;
  for (i = 0; i < len; i++)
    if (!font->get_glyph (decomposed[i], 0, &glyph))
      return false;

  for (i = 0; i < len; i++)
    output_glyph (buffer, decomposed[i]);

  return true;
}

static void
decompose_current_character (hb_font_t *font, hb_buffer_t *buffer,
			     bool shortest)
{
  hb_codepoint_t glyph;

  
  if (shortest && font->get_glyph (buffer->cur().codepoint, 0, &glyph))
    buffer->next_glyph ();
  else if (decompose (font, buffer, shortest, buffer->cur().codepoint))
    buffer->skip_glyph ();
  else if (!shortest && font->get_glyph (buffer->cur().codepoint, 0, &glyph))
    buffer->next_glyph ();
  else if (decompose_compatibility (font, buffer, buffer->cur().codepoint))
    buffer->skip_glyph ();
  else
    buffer->next_glyph ();
}

static void
decompose_multi_char_cluster (hb_font_t *font, hb_buffer_t *buffer,
			      unsigned int end)
{
  
  for (unsigned int i = buffer->idx; i < end; i++)
    if (unlikely (buffer->unicode->is_variation_selector (buffer->info[i].codepoint))) {
      while (buffer->idx < end)
	buffer->next_glyph ();
      return;
    }

  while (buffer->idx < end)
    decompose_current_character (font, buffer, false);
}

static int
compare_combining_class (const hb_glyph_info_t *pa, const hb_glyph_info_t *pb)
{
  unsigned int a = _hb_glyph_info_get_modified_combining_class (pa);
  unsigned int b = _hb_glyph_info_get_modified_combining_class (pb);

  return a < b ? -1 : a == b ? 0 : +1;
}

void
_hb_ot_shape_normalize (hb_font_t *font, hb_buffer_t *buffer,
			hb_ot_shape_normalization_mode_t mode)
{
  bool recompose = mode != HB_OT_SHAPE_NORMALIZATION_MODE_DECOMPOSED;
  bool has_multichar_clusters = false;
  unsigned int count;

  






  

  buffer->clear_output ();
  count = buffer->len;
  for (buffer->idx = 0; buffer->idx < count;)
  {
    unsigned int end;
    for (end = buffer->idx + 1; end < count; end++)
      if (buffer->cur().cluster != buffer->info[end].cluster)
        break;

    if (buffer->idx + 1 == end)
      decompose_current_character (font, buffer, recompose);
    else {
      decompose_multi_char_cluster (font, buffer, end);
      has_multichar_clusters = true;
    }
  }
  buffer->swap_buffers ();


  if (mode != HB_OT_SHAPE_NORMALIZATION_MODE_COMPOSED_FULL && !has_multichar_clusters)
    return; 


  

  count = buffer->len;
  for (unsigned int i = 0; i < count; i++)
  {
    if (_hb_glyph_info_get_modified_combining_class (&buffer->info[i]) == 0)
      continue;

    unsigned int end;
    for (end = i + 1; end < count; end++)
      if (_hb_glyph_info_get_modified_combining_class (&buffer->info[end]) == 0)
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
	 _hb_glyph_info_get_modified_combining_class (&buffer->cur()) != 0) &&
	

	(starter == buffer->out_len - 1 ||
	 _hb_glyph_info_get_modified_combining_class (&buffer->prev()) < _hb_glyph_info_get_modified_combining_class (&buffer->cur())) &&
	
	buffer->unicode->compose (buffer->out_info[starter].codepoint,
				  buffer->cur().codepoint,
				  &composed) &&
	
	font->get_glyph (composed, 0, &glyph))
    {
      
      buffer->next_glyph (); 
      if (unlikely (buffer->in_error))
        return;
      buffer->merge_out_clusters (starter, buffer->out_len);
      buffer->out_len--; 
      buffer->out_info[starter].codepoint = composed; 
      _hb_glyph_info_set_unicode_props (&buffer->out_info[starter], buffer->unicode);

      continue;
    }

    
    buffer->next_glyph ();

    if (_hb_glyph_info_get_modified_combining_class (&buffer->prev()) == 0)
      starter = buffer->out_len - 1;
  }
  buffer->swap_buffers ();

}
