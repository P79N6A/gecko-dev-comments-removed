

























#include "hb-ot-shape-complex-private.hh"




#define indic_category() complex_var_persistent_u8_0() /* indic_category_t */
#define indic_position() complex_var_persistent_u8_1() /* indic_matra_category_t */

#define INDIC_TABLE_ELEMENT_TYPE uint8_t






enum indic_category_t {
  OT_X = 0,
  OT_C,
  OT_Ra, 
  OT_V,
  OT_N,
  OT_H,
  OT_ZWNJ,
  OT_ZWJ,
  OT_M,
  OT_SM,
  OT_VD,
  OT_A,
  OT_NBSP
};


enum indic_position_t {
  POS_PRE,
  POS_BASE,
  POS_ABOVE,
  POS_BELOW,
  POS_POST
};



enum indic_syllabic_category_t {
  INDIC_SYLLABIC_CATEGORY_OTHER			= OT_X,

  INDIC_SYLLABIC_CATEGORY_AVAGRAHA		= OT_X,
  INDIC_SYLLABIC_CATEGORY_BINDU			= OT_SM,
  INDIC_SYLLABIC_CATEGORY_CONSONANT		= OT_C,
  INDIC_SYLLABIC_CATEGORY_CONSONANT_DEAD	= OT_C,
  INDIC_SYLLABIC_CATEGORY_CONSONANT_FINAL	= OT_C,
  INDIC_SYLLABIC_CATEGORY_CONSONANT_HEAD_LETTER	= OT_C,
  INDIC_SYLLABIC_CATEGORY_CONSONANT_MEDIAL	= OT_C,
  INDIC_SYLLABIC_CATEGORY_CONSONANT_PLACEHOLDER	= OT_NBSP,
  INDIC_SYLLABIC_CATEGORY_CONSONANT_SUBJOINED	= OT_C,
  INDIC_SYLLABIC_CATEGORY_CONSONANT_REPHA	= OT_C,
  INDIC_SYLLABIC_CATEGORY_MODIFYING_LETTER	= OT_X,
  INDIC_SYLLABIC_CATEGORY_NUKTA			= OT_N,
  INDIC_SYLLABIC_CATEGORY_REGISTER_SHIFTER	= OT_X,
  INDIC_SYLLABIC_CATEGORY_TONE_LETTER		= OT_X,
  INDIC_SYLLABIC_CATEGORY_TONE_MARK		= OT_X,
  INDIC_SYLLABIC_CATEGORY_VIRAMA		= OT_H,
  INDIC_SYLLABIC_CATEGORY_VISARGA		= OT_SM,
  INDIC_SYLLABIC_CATEGORY_VOWEL			= OT_V,
  INDIC_SYLLABIC_CATEGORY_VOWEL_DEPENDENT	= OT_M,
  INDIC_SYLLABIC_CATEGORY_VOWEL_INDEPENDENT	= OT_V
};


enum indic_matra_category_t {
  INDIC_MATRA_CATEGORY_NOT_APPLICABLE		= POS_BASE,

  INDIC_MATRA_CATEGORY_LEFT			= POS_PRE,
  INDIC_MATRA_CATEGORY_TOP			= POS_ABOVE,
  INDIC_MATRA_CATEGORY_BOTTOM			= POS_BELOW,
  INDIC_MATRA_CATEGORY_RIGHT			= POS_POST,

  




  INDIC_MATRA_CATEGORY_BOTTOM_AND_RIGHT		= INDIC_MATRA_CATEGORY_BOTTOM,
  INDIC_MATRA_CATEGORY_LEFT_AND_RIGHT		= INDIC_MATRA_CATEGORY_LEFT,
  INDIC_MATRA_CATEGORY_TOP_AND_BOTTOM		= INDIC_MATRA_CATEGORY_BOTTOM,
  INDIC_MATRA_CATEGORY_TOP_AND_BOTTOM_AND_RIGHT	= INDIC_MATRA_CATEGORY_BOTTOM,
  INDIC_MATRA_CATEGORY_TOP_AND_LEFT		= INDIC_MATRA_CATEGORY_LEFT,
  INDIC_MATRA_CATEGORY_TOP_AND_LEFT_AND_RIGHT	= INDIC_MATRA_CATEGORY_LEFT,
  INDIC_MATRA_CATEGORY_TOP_AND_RIGHT		= INDIC_MATRA_CATEGORY_RIGHT,

  INDIC_MATRA_CATEGORY_INVISIBLE		= INDIC_MATRA_CATEGORY_NOT_APPLICABLE,
  INDIC_MATRA_CATEGORY_OVERSTRUCK		= INDIC_MATRA_CATEGORY_NOT_APPLICABLE,
  INDIC_MATRA_CATEGORY_VISUAL_ORDER_LEFT	= INDIC_MATRA_CATEGORY_NOT_APPLICABLE
};



#define INDIC_COMBINE_CATEGORIES(S,M) \
  (ASSERT_STATIC_EXPR_ZERO (M == INDIC_MATRA_CATEGORY_NOT_APPLICABLE || (S == INDIC_SYLLABIC_CATEGORY_VIRAMA || S == INDIC_SYLLABIC_CATEGORY_VOWEL_DEPENDENT)) + \
   ASSERT_STATIC_EXPR_ZERO (S < 16 && M < 16) + \
   ((M << 4) | S))

#include "hb-ot-shape-complex-indic-table.hh"







static const struct consonant_position_t {
  hb_codepoint_t u;
  indic_position_t position;
} consonant_positions[] = {
  {0x0930, POS_BELOW},
  {0x09AC, POS_BELOW},
  {0x09AF, POS_POST},
  {0x09B0, POS_BELOW},
  {0x09F0, POS_BELOW},
  {0x0A2F, POS_POST},
  {0x0A30, POS_BELOW},
  {0x0A35, POS_BELOW},
  {0x0A39, POS_BELOW},
  {0x0AB0, POS_BELOW},
  {0x0B24, POS_BELOW},
  {0x0B28, POS_BELOW},
  {0x0B2C, POS_BELOW},
  {0x0B2D, POS_BELOW},
  {0x0B2E, POS_BELOW},
  {0x0B2F, POS_POST},
  {0x0B30, POS_BELOW},
  {0x0B32, POS_BELOW},
  {0x0B33, POS_BELOW},
  {0x0B5F, POS_POST},
  {0x0B71, POS_BELOW},
  {0x0C15, POS_BELOW},
  {0x0C16, POS_BELOW},
  {0x0C17, POS_BELOW},
  {0x0C18, POS_BELOW},
  {0x0C19, POS_BELOW},
  {0x0C1A, POS_BELOW},
  {0x0C1B, POS_BELOW},
  {0x0C1C, POS_BELOW},
  {0x0C1D, POS_BELOW},
  {0x0C1E, POS_BELOW},
  {0x0C1F, POS_BELOW},
  {0x0C20, POS_BELOW},
  {0x0C21, POS_BELOW},
  {0x0C22, POS_BELOW},
  {0x0C23, POS_BELOW},
  {0x0C24, POS_BELOW},
  {0x0C25, POS_BELOW},
  {0x0C26, POS_BELOW},
  {0x0C27, POS_BELOW},
  {0x0C28, POS_BELOW},
  {0x0C2A, POS_BELOW},
  {0x0C2B, POS_BELOW},
  {0x0C2C, POS_BELOW},
  {0x0C2D, POS_BELOW},
  {0x0C2E, POS_BELOW},
  {0x0C2F, POS_BELOW},
  {0x0C30, POS_BELOW},
  {0x0C32, POS_BELOW},
  {0x0C33, POS_BELOW},
  {0x0C35, POS_BELOW},
  {0x0C36, POS_BELOW},
  {0x0C37, POS_BELOW},
  {0x0C38, POS_BELOW},
  {0x0C39, POS_BELOW},
  {0x0C95, POS_BELOW},
  {0x0C96, POS_BELOW},
  {0x0C97, POS_BELOW},
  {0x0C98, POS_BELOW},
  {0x0C99, POS_BELOW},
  {0x0C9A, POS_BELOW},
  {0x0C9B, POS_BELOW},
  {0x0C9C, POS_BELOW},
  {0x0C9D, POS_BELOW},
  {0x0C9E, POS_BELOW},
  {0x0C9F, POS_BELOW},
  {0x0CA0, POS_BELOW},
  {0x0CA1, POS_BELOW},
  {0x0CA2, POS_BELOW},
  {0x0CA3, POS_BELOW},
  {0x0CA4, POS_BELOW},
  {0x0CA5, POS_BELOW},
  {0x0CA6, POS_BELOW},
  {0x0CA7, POS_BELOW},
  {0x0CA8, POS_BELOW},
  {0x0CAA, POS_BELOW},
  {0x0CAB, POS_BELOW},
  {0x0CAC, POS_BELOW},
  {0x0CAD, POS_BELOW},
  {0x0CAE, POS_BELOW},
  {0x0CAF, POS_BELOW},
  {0x0CB0, POS_BELOW},
  {0x0CB2, POS_BELOW},
  {0x0CB3, POS_BELOW},
  {0x0CB5, POS_BELOW},
  {0x0CB6, POS_BELOW},
  {0x0CB7, POS_BELOW},
  {0x0CB8, POS_BELOW},
  {0x0CB9, POS_BELOW},
  {0x0CDE, POS_BELOW},
  {0x0D2F, POS_POST},
  {0x0D30, POS_POST},
  {0x0D32, POS_BELOW},
  {0x0D35, POS_POST},
};




static const hb_codepoint_t ra_chars[] = {
  0x0930, 
  0x09B0, 
  0x09F0, 


  0x0AB0, 
  0x0B30, 


  0x0CB0, 

};

static int
compare_codepoint (const void *pa, const void *pb)
{
  hb_codepoint_t a = * (hb_codepoint_t *) pa;
  hb_codepoint_t b = * (hb_codepoint_t *) pb;

  return a < b ? -1 : a == b ? 0 : +1;
}

static indic_position_t
consonant_position (hb_codepoint_t u)
{
  consonant_position_t *record;

  record = (consonant_position_t *) bsearch (&u, consonant_positions,
					     ARRAY_LENGTH (consonant_positions),
					     sizeof (consonant_positions[0]),
					     compare_codepoint);

  return record ? record->position : POS_BASE;
}

static bool
is_ra (hb_codepoint_t u)
{
  return !!bsearch (&u, ra_chars,
		    ARRAY_LENGTH (ra_chars),
		    sizeof (ra_chars[0]),
		    compare_codepoint);
}

static bool
is_joiner (const hb_glyph_info_t &info)
{
  return !!(FLAG (info.indic_category()) & (FLAG (OT_ZWJ) | FLAG (OT_ZWNJ)));
}

static bool
is_consonant (const hb_glyph_info_t &info)
{
  return !!(FLAG (info.indic_category()) & (FLAG (OT_C) | FLAG (OT_Ra)));
}

static const struct {
  hb_tag_t tag;
  hb_bool_t is_global;
} indic_basic_features[] =
{
  {HB_TAG('n','u','k','t'), true},
  {HB_TAG('a','k','h','n'), false},
  {HB_TAG('r','p','h','f'), false},
  {HB_TAG('r','k','r','f'), false},
  {HB_TAG('p','r','e','f'), false},
  {HB_TAG('b','l','w','f'), false},
  {HB_TAG('h','a','l','f'), false},
  {HB_TAG('v','a','t','u'), true},
  {HB_TAG('p','s','t','f'), false},
  {HB_TAG('c','j','c','t'), false},
};


enum {
  _NUKT,
  AKHN,
  RPHF,
  RKRF,
  PREF,
  BLWF,
  HALF,
  _VATU,
  PSTF,
  CJCT
};

static const hb_tag_t indic_other_features[] =
{
  HB_TAG('p','r','e','s'),
  HB_TAG('a','b','v','s'),
  HB_TAG('b','l','w','s'),
  HB_TAG('p','s','t','s'),
  HB_TAG('h','a','l','n'),

  HB_TAG('d','i','s','t'),
  HB_TAG('a','b','v','m'),
  HB_TAG('b','l','w','m'),
};


static void
initial_reordering (const hb_ot_map_t *map,
		    hb_face_t *face,
		    hb_buffer_t *buffer,
		    void *user_data HB_UNUSED);
static void
final_reordering (const hb_ot_map_t *map,
		  hb_face_t *face,
		  hb_buffer_t *buffer,
		  void *user_data HB_UNUSED);

void
_hb_ot_shape_complex_collect_features_indic (hb_ot_map_builder_t *map, const hb_segment_properties_t  *props)
{
  map->add_bool_feature (HB_TAG('l','o','c','l'));
  

  map->add_bool_feature (HB_TAG('c','c','m','p'));

  map->add_gsub_pause (initial_reordering, NULL);

  for (unsigned int i = 0; i < ARRAY_LENGTH (indic_basic_features); i++)
    map->add_bool_feature (indic_basic_features[i].tag, indic_basic_features[i].is_global);

  map->add_gsub_pause (final_reordering, NULL);

  for (unsigned int i = 0; i < ARRAY_LENGTH (indic_other_features); i++)
    map->add_bool_feature (indic_other_features[i], true);
}


bool
_hb_ot_shape_complex_prefer_decomposed_indic (void)
{
  
  return TRUE;
}


void
_hb_ot_shape_complex_setup_masks_indic (hb_ot_map_t *map, hb_buffer_t *buffer)
{
  HB_BUFFER_ALLOCATE_VAR (buffer, indic_category);
  HB_BUFFER_ALLOCATE_VAR (buffer, indic_position);

  


  unsigned int count = buffer->len;
  for (unsigned int i = 0; i < count; i++)
  {
    unsigned int type = get_indic_categories (buffer->info[i].codepoint);

    buffer->info[i].indic_category() = type & 0x0F;
    buffer->info[i].indic_position() = type >> 4;

    if (buffer->info[i].indic_category() == OT_C) {
      buffer->info[i].indic_position() = consonant_position (buffer->info[i].codepoint);
      if (is_ra (buffer->info[i].codepoint))
	buffer->info[i].indic_category() = OT_Ra;
    } else if (buffer->info[i].codepoint == 0x200C)
      buffer->info[i].indic_category() = OT_ZWNJ;
    else if (buffer->info[i].codepoint == 0x200D)
      buffer->info[i].indic_category() = OT_ZWJ;
  }
}

static int
compare_indic_order (const hb_glyph_info_t *pa, const hb_glyph_info_t *pb)
{
  int a = pa->indic_position();
  int b = pb->indic_position();

  return a < b ? -1 : a == b ? 0 : +1;
}

static void
found_consonant_syllable (const hb_ot_map_t *map, hb_buffer_t *buffer, hb_mask_t *mask_array,
			  unsigned int start, unsigned int end)
{
  unsigned int i;
  hb_glyph_info_t *info = buffer->info;

  


  













  unsigned int base = end;

  
  i = end;
  unsigned int limit = start;
  if (info[start].indic_category() == OT_Ra && start + 2 <= end) {
    limit += 2;
    base = start;
  };
  do {
    i--;
    
    if (is_consonant (info[i]))
    {
      

      if (info[i].indic_position() != POS_BELOW &&
	  info[i].indic_position() != POS_POST)
      {
        base = i;
	break;
      }

      




      








      

      base = i;
    }
    else
      if (is_joiner (info[i]))
        break;
  } while (i > limit);
  if (base < start)
    base = start; 


  




















  










  

  for (i = start; i < base; i++)
    info[i].indic_position() = POS_PRE;
  info[base].indic_position() = POS_BASE;


  
  if (start + 3 <= end &&
      info[start].indic_category() == OT_Ra &&
      info[start + 1].indic_category() == OT_H &&
      !is_joiner (info[start + 2]))
   {
    info[start].indic_position() = POS_POST;
    info[start].mask = mask_array[RPHF];
   }

  

  if ((map->get_chosen_script (0) & 0x000000FF) != '2') {
    
    for (i = base + 1; i < end; i++)
      if (info[i].indic_category() == OT_H) {
        unsigned int j;
        for (j = end - 1; j > i; j--)
	  if ((FLAG (info[j].indic_category()) & (FLAG (OT_C) | FLAG (OT_Ra))))
	    break;
	if (j > i) {
	  
	  hb_glyph_info_t t = info[i];
	  memmove (&info[i], &info[i + 1], (j - i) * sizeof (info[0]));
	  info[j] = t;
	}
        break;
      }
  }

  
  for (i = start + 1; i < end; i++)
    if ((FLAG (info[i].indic_category()) &
	 (FLAG (OT_ZWNJ) | FLAG (OT_ZWJ) | FLAG (OT_N) | FLAG (OT_H))))
      info[i].indic_position() = info[i - 1].indic_position();

  
  if (end - start > 20)
    return;

  
  hb_bubble_sort (info + start, end - start, compare_indic_order);

  

  {
    hb_mask_t mask;

    
    mask = mask_array[HALF] | mask_array[AKHN] | mask_array[CJCT];
    for (i = start; i < base; i++)
      info[i].mask  |= mask;
    
    mask = mask_array[AKHN] | mask_array[CJCT];
    info[base].mask |= mask;
    
    mask = mask_array[BLWF] | mask_array[PSTF] | mask_array[CJCT];
    for (i = base + 1; i < end; i++)
      info[i].mask  |= mask;
  }

  
  for (i = start + 1; i < end; i++)
    if (is_joiner (info[i])) {
      bool non_joiner = info[i].indic_category() == OT_ZWNJ;
      unsigned int j = i;

      do {
	j--;

	




	
	if (non_joiner)
	  info[j].mask &= !mask_array[HALF];

      } while (j > start && !is_consonant (info[j]));
    }
}


static void
found_vowel_syllable (const hb_ot_map_t *map, hb_buffer_t *buffer, hb_mask_t *mask_array,
		      unsigned int start, unsigned int end)
{
  



}

static void
found_standalone_cluster (const hb_ot_map_t *map, hb_buffer_t *buffer, hb_mask_t *mask_array,
			  unsigned int start, unsigned int end)
{
  



}

static void
found_non_indic (const hb_ot_map_t *map, hb_buffer_t *buffer, hb_mask_t *mask_array,
		 unsigned int start, unsigned int end)
{
  

}

#include "hb-ot-shape-complex-indic-machine.hh"

static void
remove_joiners (hb_buffer_t *buffer)
{
  



  buffer->clear_output ();
  unsigned int count = buffer->len;
  for (buffer->idx = 0; buffer->idx < count;)
    if (unlikely (is_joiner (buffer->info[buffer->idx])))
      buffer->skip_glyph ();
    else
      buffer->next_glyph ();

  buffer->swap_buffers ();
}

static void
initial_reordering (const hb_ot_map_t *map,
		    hb_face_t *face,
		    hb_buffer_t *buffer,
		    void *user_data HB_UNUSED)
{
  hb_mask_t mask_array[ARRAY_LENGTH (indic_basic_features)] = {0};
  unsigned int num_masks = ARRAY_LENGTH (indic_basic_features);
  for (unsigned int i = 0; i < num_masks; i++)
    mask_array[i] = map->get_1_mask (indic_basic_features[i].tag);

  find_syllables (map, buffer, mask_array);

  remove_joiners (buffer);
}

static void
final_reordering (const hb_ot_map_t *map,
		  hb_face_t *face,
		  hb_buffer_t *buffer,
		  void *user_data HB_UNUSED)
{
  








































































  



  HB_BUFFER_DEALLOCATE_VAR (buffer, indic_category);
  HB_BUFFER_DEALLOCATE_VAR (buffer, indic_position);
}



