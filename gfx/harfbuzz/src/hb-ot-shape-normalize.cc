

























#include "hb-ot-shape-private.hh"
#include "hb-ot-shape-complex-private.hh"








































static void
output_glyph (hb_ot_shape_context_t *c,
	      hb_codepoint_t glyph)
{
  hb_buffer_t *buffer = c->buffer;

  buffer->output_glyph (glyph);
  hb_glyph_info_set_unicode_props (&buffer->out_info[buffer->out_len - 1], buffer->unicode);
}

static bool
decompose (hb_ot_shape_context_t *c,
	   bool shortest,
	   hb_codepoint_t ab)
{
  hb_codepoint_t a, b, glyph;

  if (!hb_unicode_decompose (c->buffer->unicode, ab, &a, &b) ||
      (b && !hb_font_get_glyph (c->font, b, 0, &glyph)))
    return FALSE;

  bool has_a = hb_font_get_glyph (c->font, a, 0, &glyph);
  if (shortest && has_a) {
    
    output_glyph (c, a);
    if (b)
      output_glyph (c, b);
    return TRUE;
  }

  if (decompose (c, shortest, a)) {
    if (b)
      output_glyph (c, b);
    return TRUE;
  }

  if (has_a) {
    output_glyph (c, a);
    if (b)
      output_glyph (c, b);
    return TRUE;
  }

  return FALSE;
}

static void
decompose_current_glyph (hb_ot_shape_context_t *c,
			 bool shortest)
{
  if (decompose (c, shortest, c->buffer->info[c->buffer->idx].codepoint))
    c->buffer->skip_glyph ();
  else
    c->buffer->next_glyph ();
}

static void
decompose_single_char_cluster (hb_ot_shape_context_t *c,
			       bool will_recompose)
{
  hb_codepoint_t glyph;

  
  if (will_recompose && hb_font_get_glyph (c->font, c->buffer->info[c->buffer->idx].codepoint, 0, &glyph)) {
    c->buffer->next_glyph ();
    return;
  }

  decompose_current_glyph (c, will_recompose);
}

static void
decompose_multi_char_cluster (hb_ot_shape_context_t *c,
			      unsigned int end)
{
  
  for (unsigned int i = c->buffer->idx; i < end; i++)
    if (unlikely (is_variation_selector (c->buffer->info[i].codepoint))) {
      while (c->buffer->idx < end)
	c->buffer->next_glyph ();
      return;
    }

  while (c->buffer->idx < end)
    decompose_current_glyph (c, FALSE);
}

static int
compare_combining_class (const hb_glyph_info_t *pa, const hb_glyph_info_t *pb)
{
  unsigned int a = pa->combining_class();
  unsigned int b = pb->combining_class();

  return a < b ? -1 : a == b ? 0 : +1;
}

void
_hb_ot_shape_normalize (hb_ot_shape_context_t *c)
{
  hb_buffer_t *buffer = c->buffer;
  bool recompose = !hb_ot_shape_complex_prefer_decomposed (c->plan->shaper);
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
      decompose_single_char_cluster (c, recompose);
    else {
      decompose_multi_char_cluster (c, end);
      has_multichar_clusters = TRUE;
    }
  }
  buffer->swap_buffers ();


  










  if (!has_multichar_clusters)
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
    if (buffer->info[buffer->idx].combining_class() == 0) {
      starter = buffer->out_len;
      buffer->next_glyph ();
      continue;
    }

    hb_codepoint_t composed, glyph;
    if ((buffer->out_info[buffer->out_len - 1].combining_class() >=
	 buffer->info[buffer->idx].combining_class()) ||
	!hb_unicode_compose (c->buffer->unicode,
			     buffer->out_info[starter].codepoint,
			     buffer->info[buffer->idx].codepoint,
			     &composed) ||
	!hb_font_get_glyph (c->font, composed, 0, &glyph))
    {
      
      buffer->next_glyph ();
      continue;
    }

    
    buffer->out_info[starter].codepoint = composed;
    hb_glyph_info_set_unicode_props (&buffer->out_info[starter], buffer->unicode);

    buffer->skip_glyph ();
  }
  buffer->swap_buffers ();

}

