

























#include "hb-ot-shape-normalize-private.hh"
#include "hb-ot-shape-complex-private.hh"
#include "hb-ot-shape-private.hh"























































static bool
decompose_unicode (const hb_ot_shape_normalize_context_t *c,
		   hb_codepoint_t  ab,
		   hb_codepoint_t *a,
		   hb_codepoint_t *b)
{
  return c->unicode->decompose (ab, a, b);
}

static bool
compose_unicode (const hb_ot_shape_normalize_context_t *c,
		 hb_codepoint_t  a,
		 hb_codepoint_t  b,
		 hb_codepoint_t *ab)
{
  return c->unicode->compose (a, b, ab);
}

static inline void
set_glyph (hb_glyph_info_t &info, hb_font_t *font)
{
  font->get_glyph (info.codepoint, 0, &info.glyph_index());
}

static inline void
output_char (hb_buffer_t *buffer, hb_codepoint_t unichar, hb_codepoint_t glyph)
{
  buffer->cur().glyph_index() = glyph;
  buffer->output_glyph (unichar);
  _hb_glyph_info_set_unicode_props (&buffer->prev(), buffer->unicode);
}

static inline void
next_char (hb_buffer_t *buffer, hb_codepoint_t glyph)
{
  buffer->cur().glyph_index() = glyph;
  buffer->next_glyph ();
}

static inline void
skip_char (hb_buffer_t *buffer)
{
  buffer->skip_glyph ();
}


static inline unsigned int
decompose (const hb_ot_shape_normalize_context_t *c, bool shortest, hb_codepoint_t ab)
{
  hb_codepoint_t a, b, a_glyph, b_glyph;

  if (!c->decompose (c, ab, &a, &b) ||
      (b && !c->font->get_glyph (b, 0, &b_glyph)))
    return 0;

  bool has_a = c->font->get_glyph (a, 0, &a_glyph);
  if (shortest && has_a) {
    
    output_char (c->buffer, a, a_glyph);
    if (likely (b)) {
      output_char (c->buffer, b, b_glyph);
      return 2;
    }
    return 1;
  }

  unsigned int ret;
  if ((ret = decompose (c, shortest, a))) {
    if (b) {
      output_char (c->buffer, b, b_glyph);
      return ret + 1;
    }
    return ret;
  }

  if (has_a) {
    output_char (c->buffer, a, a_glyph);
    if (likely (b)) {
      output_char (c->buffer, b, b_glyph);
      return 2;
    }
    return 1;
  }

  return 0;
}


static inline bool
decompose_compatibility (const hb_ot_shape_normalize_context_t *c, hb_codepoint_t u)
{
  unsigned int len, i;
  hb_codepoint_t decomposed[HB_UNICODE_MAX_DECOMPOSITION_LEN];
  hb_codepoint_t glyphs[HB_UNICODE_MAX_DECOMPOSITION_LEN];

  len = c->buffer->unicode->decompose_compatibility (u, decomposed);
  if (!len)
    return 0;

  for (i = 0; i < len; i++)
    if (!c->font->get_glyph (decomposed[i], 0, &glyphs[i]))
      return 0;

  for (i = 0; i < len; i++)
    output_char (c->buffer, decomposed[i], glyphs[i]);

  return len;
}


static inline void
decompose_current_character (const hb_ot_shape_normalize_context_t *c, bool shortest)
{
  hb_buffer_t * const buffer = c->buffer;
  hb_codepoint_t glyph;

  
  if (shortest && c->font->get_glyph (buffer->cur().codepoint, 0, &glyph))
    next_char (buffer, glyph);
  else if (decompose (c, shortest, buffer->cur().codepoint))
    skip_char (buffer);
  else if (!shortest && c->font->get_glyph (buffer->cur().codepoint, 0, &glyph))
    next_char (buffer, glyph);
  else if (decompose_compatibility (c, buffer->cur().codepoint))
    skip_char (buffer);
  else
    next_char (buffer, glyph); 
}

static inline void
handle_variation_selector_cluster (const hb_ot_shape_normalize_context_t *c, unsigned int end)
{
  hb_buffer_t * const buffer = c->buffer;
  for (; buffer->idx < end - 1;) {
    if (unlikely (buffer->unicode->is_variation_selector (buffer->cur(+1).codepoint))) {
      
      c->font->get_glyph (buffer->cur().codepoint, buffer->cur(+1).codepoint, &buffer->cur().glyph_index());
      buffer->replace_glyphs (2, 1, &buffer->cur().codepoint);
    } else {
      set_glyph (buffer->cur(), c->font);
      buffer->next_glyph ();
    }
  }
  if (likely (buffer->idx < end)) {
    set_glyph (buffer->cur(), c->font);
    buffer->next_glyph ();
  }
}


static inline void
decompose_multi_char_cluster (const hb_ot_shape_normalize_context_t *c, unsigned int end)
{
  hb_buffer_t * const buffer = c->buffer;
  
  for (unsigned int i = buffer->idx; i < end; i++)
    if (unlikely (buffer->unicode->is_variation_selector (buffer->info[i].codepoint))) {
      handle_variation_selector_cluster (c, end);
      return;
    }

  while (buffer->idx < end)
    decompose_current_character (c, false);
}

static inline void
decompose_cluster (const hb_ot_shape_normalize_context_t *c, bool short_circuit, unsigned int end)
{
  if (likely (c->buffer->idx + 1 == end))
    decompose_current_character (c, short_circuit);
  else
    decompose_multi_char_cluster (c, end);
}


static int
compare_combining_class (const hb_glyph_info_t *pa, const hb_glyph_info_t *pb)
{
  unsigned int a = _hb_glyph_info_get_modified_combining_class (pa);
  unsigned int b = _hb_glyph_info_get_modified_combining_class (pb);

  return a < b ? -1 : a == b ? 0 : +1;
}


void
_hb_ot_shape_normalize (const hb_ot_shape_plan_t *plan,
			hb_buffer_t *buffer,
			hb_font_t *font)
{
  hb_ot_shape_normalization_mode_t mode = plan->shaper->normalization_preference ?
					  plan->shaper->normalization_preference (&buffer->props) :
					  HB_OT_SHAPE_NORMALIZATION_MODE_DEFAULT;
  const hb_ot_shape_normalize_context_t c = {
    plan,
    buffer,
    font,
    buffer->unicode,
    plan->shaper->decompose ? plan->shaper->decompose : decompose_unicode,
    plan->shaper->compose   ? plan->shaper->compose   : compose_unicode
  };

  bool short_circuit = mode != HB_OT_SHAPE_NORMALIZATION_MODE_DECOMPOSED &&
		       mode != HB_OT_SHAPE_NORMALIZATION_MODE_COMPOSED_DIACRITICS_NO_SHORT_CIRCUIT;
  unsigned int count;

  






  

  buffer->clear_output ();
  count = buffer->len;
  for (buffer->idx = 0; buffer->idx < count;)
  {
    unsigned int end;
    for (end = buffer->idx + 1; end < count; end++)
      if (buffer->cur().cluster != buffer->info[end].cluster)
        break;

    decompose_cluster (&c, short_circuit, end);
  }
  buffer->swap_buffers ();


  

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


  if (mode == HB_OT_SHAPE_NORMALIZATION_MODE_DECOMPOSED)
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
	 HB_UNICODE_GENERAL_CATEGORY_IS_MARK (_hb_glyph_info_get_general_category (&buffer->cur()))) &&
	

	(starter == buffer->out_len - 1 ||
	 _hb_glyph_info_get_modified_combining_class (&buffer->prev()) < _hb_glyph_info_get_modified_combining_class (&buffer->cur())) &&
	
	c.compose (&c,
		   buffer->out_info[starter].codepoint,
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
      set_glyph (buffer->out_info[starter], font);
      _hb_glyph_info_set_unicode_props (&buffer->out_info[starter], buffer->unicode);

      continue;
    }

    
    buffer->next_glyph ();

    if (_hb_glyph_info_get_modified_combining_class (&buffer->prev()) == 0)
      starter = buffer->out_len - 1;
  }
  buffer->swap_buffers ();

}
