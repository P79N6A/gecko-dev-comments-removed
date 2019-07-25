

























#include "hb-ot-shape-normalize-private.hh"
#include "hb-ot-shape-private.hh"























































static hb_bool_t
decompose_func (hb_unicode_funcs_t *unicode,
		hb_codepoint_t  ab,
		hb_codepoint_t *a,
		hb_codepoint_t *b)
{
  
  switch (ab) {
    case 0x0AC9  : return false;

    case 0x0931  : return false;
    case 0x0B94  : return false;

    

    case 0x0DDA  : *a = 0x0DD9; *b= 0x0DDA; return true;
    case 0x0DDC  : *a = 0x0DD9; *b= 0x0DDC; return true;
    case 0x0DDD  : *a = 0x0DD9; *b= 0x0DDD; return true;
    case 0x0DDE  : *a = 0x0DD9; *b= 0x0DDE; return true;

    case 0x0F77  : *a = 0x0FB2; *b= 0x0F81; return true;
    case 0x0F79  : *a = 0x0FB3; *b= 0x0F81; return true;
    case 0x17BE  : *a = 0x17C1; *b= 0x17BE; return true;
    case 0x17BF  : *a = 0x17C1; *b= 0x17BF; return true;
    case 0x17C0  : *a = 0x17C1; *b= 0x17C0; return true;
    case 0x17C4  : *a = 0x17C1; *b= 0x17C4; return true;
    case 0x17C5  : *a = 0x17C1; *b= 0x17C5; return true;
    case 0x1925  : *a = 0x1920; *b= 0x1923; return true;
    case 0x1926  : *a = 0x1920; *b= 0x1924; return true;
    case 0x1B3C  : *a = 0x1B42; *b= 0x1B3C; return true;
    case 0x1112E  : *a = 0x11127; *b= 0x11131; return true;
    case 0x1112F  : *a = 0x11127; *b= 0x11132; return true;
#if 0
    case 0x0B57  : *a = 0xno decomp, -> RIGHT; return true;
    case 0x1C29  : *a = 0xno decomp, -> LEFT; return true;
    case 0xA9C0  : *a = 0xno decomp, -> RIGHT; return true;
    case 0x111BF  : *a = 0xno decomp, -> ABOVE; return true;
#endif
  }
  return unicode->decompose (ab, a, b);
}

static hb_bool_t
compose_func (hb_unicode_funcs_t *unicode,
	      hb_codepoint_t  a,
	      hb_codepoint_t  b,
	      hb_codepoint_t *ab)
{
  
  if ((FLAG (unicode->general_category (a)) &
       (FLAG (HB_UNICODE_GENERAL_CATEGORY_SPACING_MARK) |
	FLAG (HB_UNICODE_GENERAL_CATEGORY_ENCLOSING_MARK) |
	FLAG (HB_UNICODE_GENERAL_CATEGORY_NON_SPACING_MARK))))
    return false;
  
  if (a == 0x09AF && b == 0x09BC) { *ab = 0x09DF; return true; }

  
  

  
  
  static const hb_codepoint_t sDageshForms[0x05EA - 0x05D0 + 1] = {
    0xFB30, 
    0xFB31, 
    0xFB32, 
    0xFB33, 
    0xFB34, 
    0xFB35, 
    0xFB36, 
    0, 
    0xFB38, 
    0xFB39, 
    0xFB3A, 
    0xFB3B, 
    0xFB3C, 
    0, 
    0xFB3E, 
    0, 
    0xFB40, 
    0xFB41, 
    0, 
    0xFB43, 
    0xFB44, 
    0, 
    0xFB46, 
    0xFB47, 
    0xFB48, 
    0xFB49, 
    0xFB4A 
  };

  hb_bool_t found = unicode->compose (a, b, ab);

  if (!found && (b & ~0x7F) == 0x0580) {
      
      
      switch (b) {
      case 0x05B4: 
	  if (a == 0x05D9) { 
	      *ab = 0xFB1D;
	      found = true;
	  }
	  break;
      case 0x05B7: 
	  if (a == 0x05F2) { 
	      *ab = 0xFB1F;
	      found = true;
	  } else if (a == 0x05D0) { 
	      *ab = 0xFB2E;
	      found = true;
	  }
	  break;
      case 0x05B8: 
	  if (a == 0x05D0) { 
	      *ab = 0xFB2F;
	      found = true;
	  }
	  break;
      case 0x05B9: 
	  if (a == 0x05D5) { 
	      *ab = 0xFB4B;
	      found = true;
	  }
	  break;
      case 0x05BC: 
	  if (a >= 0x05D0 && a <= 0x05EA) {
	      *ab = sDageshForms[a - 0x05D0];
	      found = (*ab != 0);
	  } else if (a == 0xFB2A) { 
	      *ab = 0xFB2C;
	      found = true;
	  } else if (a == 0xFB2B) { 
	      *ab = 0xFB2D;
	      found = true;
	  }
	  break;
      case 0x05BF: 
	  switch (a) {
	  case 0x05D1: 
	      *ab = 0xFB4C;
	      found = true;
	      break;
	  case 0x05DB: 
	      *ab = 0xFB4D;
	      found = true;
	      break;
	  case 0x05E4: 
	      *ab = 0xFB4E;
	      found = true;
	      break;
	  }
	  break;
      case 0x05C1: 
	  if (a == 0x05E9) { 
	      *ab = 0xFB2A;
	      found = true;
	  } else if (a == 0xFB49) { 
	      *ab = 0xFB2C;
	      found = true;
	  }
	  break;
      case 0x05C2: 
	  if (a == 0x05E9) { 
	      *ab = 0xFB2B;
	      found = true;
	  } else if (a == 0xFB49) { 
	      *ab = 0xFB2D;
	      found = true;
	  }
	  break;
      }
  }

  return found;
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
decompose (hb_font_t *font, hb_buffer_t *buffer, bool shortest, hb_codepoint_t ab)
{
  hb_codepoint_t a, b, a_glyph, b_glyph;

  if (!decompose_func (buffer->unicode, ab, &a, &b) ||
      (b && !font->get_glyph (b, 0, &b_glyph)))
    return 0;

  bool has_a = font->get_glyph (a, 0, &a_glyph);
  if (shortest && has_a) {
    
    output_char (buffer, a, a_glyph);
    if (likely (b)) {
      output_char (buffer, b, b_glyph);
      return 2;
    }
    return 1;
  }

  unsigned int ret;
  if ((ret = decompose (font, buffer, shortest, a))) {
    if (b) {
      output_char (buffer, b, b_glyph);
      return ret + 1;
    }
    return ret;
  }

  if (has_a) {
    output_char (buffer, a, a_glyph);
    if (likely (b)) {
      output_char (buffer, b, b_glyph);
      return 2;
    }
    return 1;
  }

  return 0;
}


static inline bool
decompose_compatibility (hb_font_t *font, hb_buffer_t *buffer, hb_codepoint_t u)
{
  unsigned int len, i;
  hb_codepoint_t decomposed[HB_UNICODE_MAX_DECOMPOSITION_LEN];
  hb_codepoint_t glyphs[HB_UNICODE_MAX_DECOMPOSITION_LEN];

  len = buffer->unicode->decompose_compatibility (u, decomposed);
  if (!len)
    return 0;

  for (i = 0; i < len; i++)
    if (!font->get_glyph (decomposed[i], 0, &glyphs[i]))
      return 0;

  for (i = 0; i < len; i++)
    output_char (buffer, decomposed[i], glyphs[i]);

  return len;
}


static inline bool
decompose_current_character (hb_font_t *font, hb_buffer_t *buffer, bool shortest)
{
  hb_codepoint_t glyph;
  unsigned int len = 1;

  
  if (shortest && font->get_glyph (buffer->cur().codepoint, 0, &glyph))
    next_char (buffer, glyph);
  else if ((len = decompose (font, buffer, shortest, buffer->cur().codepoint)))
    skip_char (buffer);
  else if (!shortest && font->get_glyph (buffer->cur().codepoint, 0, &glyph))
    next_char (buffer, glyph);
  else if ((len = decompose_compatibility (font, buffer, buffer->cur().codepoint)))
    skip_char (buffer);
  else
    next_char (buffer, glyph); 

  



  return len > 2;
}

static inline void
handle_variation_selector_cluster (hb_font_t *font, hb_buffer_t *buffer, unsigned int end)
{
  for (; buffer->idx < end - 1;) {
    if (unlikely (buffer->unicode->is_variation_selector (buffer->cur(+1).codepoint))) {
      
      font->get_glyph (buffer->cur().codepoint, buffer->cur(+1).codepoint, &buffer->cur().glyph_index());
      buffer->replace_glyphs (2, 1, &buffer->cur().codepoint);
    } else {
      set_glyph (buffer->cur(), font);
      buffer->next_glyph ();
    }
  }
  if (likely (buffer->idx < end)) {
    set_glyph (buffer->cur(), font);
    buffer->next_glyph ();
  }
}


static inline bool
decompose_multi_char_cluster (hb_font_t *font, hb_buffer_t *buffer, unsigned int end)
{
  
  for (unsigned int i = buffer->idx; i < end; i++)
    if (unlikely (buffer->unicode->is_variation_selector (buffer->info[i].codepoint))) {
      handle_variation_selector_cluster (font, buffer, end);
      return false;
    }

  while (buffer->idx < end)
    decompose_current_character (font, buffer, false);
  

  return true;
}

static inline bool
decompose_cluster (hb_font_t *font, hb_buffer_t *buffer, bool recompose, unsigned int end)
{
  if (likely (buffer->idx + 1 == end))
    return decompose_current_character (font, buffer, recompose);
  else
    return decompose_multi_char_cluster (font, buffer, end);
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
  bool can_use_recompose = false;
  unsigned int count;

  






  

  buffer->clear_output ();
  count = buffer->len;
  for (buffer->idx = 0; buffer->idx < count;)
  {
    unsigned int end;
    for (end = buffer->idx + 1; end < count; end++)
      if (buffer->cur().cluster != buffer->info[end].cluster)
        break;

    can_use_recompose = decompose_cluster (font, buffer, recompose, end) || can_use_recompose;
  }
  buffer->swap_buffers ();


  if (mode != HB_OT_SHAPE_NORMALIZATION_MODE_COMPOSED_FULL && !can_use_recompose)
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
	
	compose_func (buffer->unicode,
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
