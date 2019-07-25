

























#include "hb-ot-shape-complex-indic-private.hh"
#include "hb-ot-shape-private.hh"

static const struct indic_options_t
{
  indic_options_t (void)
  {
    char *c = getenv ("HB_OT_INDIC_OPTIONS");
    uniscribe_bug_compatible = c && strstr (c, "uniscribe-bug-compatible");
  }

  bool uniscribe_bug_compatible;
} options;

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

  return record ? record->position : POS_BASE_C;
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
  




  return !!(FLAG (info.indic_category()) & (FLAG (OT_C) | FLAG (OT_Ra) | FLAG (OT_V) | FLAG (OT_NBSP) | FLAG (OT_DOTTEDCIRCLE)));
}

struct feature_list_t {
  hb_tag_t tag;
  hb_bool_t is_global;
};

static const feature_list_t
indic_basic_features[] =
{
  {HB_TAG('n','u','k','t'), true},
  {HB_TAG('a','k','h','n'), false},
  {HB_TAG('r','p','h','f'), false},
  {HB_TAG('r','k','r','f'), true},
  {HB_TAG('p','r','e','f'), false},
  {HB_TAG('b','l','w','f'), false},
  {HB_TAG('h','a','l','f'), false},
  {HB_TAG('p','s','t','f'), false},
  {HB_TAG('c','j','c','t'), false},
  {HB_TAG('v','a','t','u'), true},
};


enum {
  _NUKT,
  AKHN,
  RPHF,
  _RKRF,
  PREF,
  BLWF,
  HALF,
  PSTF,
  CJCT,
  VATU
};

static const feature_list_t
indic_other_features[] =
{
  {HB_TAG('i','n','i','t'), false},
  {HB_TAG('p','r','e','s'), true},
  {HB_TAG('a','b','v','s'), true},
  {HB_TAG('b','l','w','s'), true},
  {HB_TAG('p','s','t','s'), true},
  {HB_TAG('h','a','l','n'), true},

  {HB_TAG('d','i','s','t'), true},
  {HB_TAG('a','b','v','m'), true},
  {HB_TAG('b','l','w','m'), true},
};


enum {
  INIT
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
_hb_ot_shape_complex_collect_features_indic (hb_ot_map_builder_t *map,
					     const hb_segment_properties_t *props HB_UNUSED)
{
  map->add_bool_feature (HB_TAG('l','o','c','l'));
  

  map->add_bool_feature (HB_TAG('c','c','m','p'));

  map->add_gsub_pause (initial_reordering, NULL);

  for (unsigned int i = 0; i < ARRAY_LENGTH (indic_basic_features); i++) {
    map->add_bool_feature (indic_basic_features[i].tag, indic_basic_features[i].is_global);
    map->add_gsub_pause (NULL, NULL);
  }

  map->add_gsub_pause (final_reordering, NULL);

  for (unsigned int i = 0; i < ARRAY_LENGTH (indic_other_features); i++) {
    map->add_bool_feature (indic_other_features[i].tag, indic_other_features[i].is_global);
    map->add_gsub_pause (NULL, NULL);
  }
}


hb_ot_shape_normalization_mode_t
_hb_ot_shape_complex_normalization_preference_indic (void)
{
  
  return HB_OT_SHAPE_NORMALIZATION_MODE_DECOMPOSED;
}


void
_hb_ot_shape_complex_setup_masks_indic (hb_ot_map_t *map HB_UNUSED,
					hb_buffer_t *buffer,
					hb_font_t *font HB_UNUSED)
{
  HB_BUFFER_ALLOCATE_VAR (buffer, indic_category);
  HB_BUFFER_ALLOCATE_VAR (buffer, indic_position);

  


  unsigned int count = buffer->len;
  for (unsigned int i = 0; i < count; i++)
  {
    hb_glyph_info_t &info = buffer->info[i];
    unsigned int type = get_indic_categories (info.codepoint);

    info.indic_category() = type & 0x0F;
    info.indic_position() = type >> 4;

    







    if (unlikely (hb_in_range<hb_codepoint_t> (info.codepoint, 0x0951, 0x0954)))
      info.indic_category() = OT_VD;

    if (info.indic_category() == OT_C) {
      info.indic_position() = consonant_position (info.codepoint);
      if (is_ra (info.codepoint))
	info.indic_category() = OT_Ra;
    } else if (info.indic_category() == OT_SM ||
	       info.indic_category() == OT_VD) {
      info.indic_position() = POS_SMVD;
    } else if (unlikely (info.codepoint == 0x200C))
      info.indic_category() = OT_ZWNJ;
    else if (unlikely (info.codepoint == 0x200D))
      info.indic_category() = OT_ZWJ;
    else if (unlikely (info.codepoint == 0x25CC))
      info.indic_category() = OT_DOTTEDCIRCLE;
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
initial_reordering_consonant_syllable (const hb_ot_map_t *map, hb_buffer_t *buffer, hb_mask_t *mask_array,
				       unsigned int start, unsigned int end)
{
  hb_glyph_info_t *info = buffer->info;


  













  unsigned int base = end;
  bool has_reph = false;

  {
    


    unsigned int limit = start;
    if (mask_array[RPHF] &&
	start + 3 <= end &&
	info[start].indic_category() == OT_Ra &&
	info[start + 1].indic_category() == OT_H &&
	!is_joiner (info[start + 2]))
    {
      limit += 2;
      base = start;
      has_reph = true;
    };

    
    unsigned int i = end;
    do {
      i--;
      
      if (is_consonant (info[i]))
      {
	

	if (info[i].indic_position() != POS_BELOW_C &&
	    info[i].indic_position() != POS_POST_C)
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


    


    if (has_reph && base == start) {
      
      has_reph = false;
    }
  }


  




















  










  

  for (unsigned int i = start; i < base; i++)
    info[i].indic_position() = POS_PRE_C;
  info[base].indic_position() = POS_BASE_C;

  
  if (has_reph)
    info[start].indic_position() = POS_RA_TO_BECOME_REPH;

  

  if ((map->get_chosen_script (0) & 0x000000FF) != '2') {
    
    for (unsigned int i = base + 1; i < end; i++)
      if (info[i].indic_category() == OT_H) {
        unsigned int j;
        for (j = end - 1; j > i; j--)
	  if (is_consonant (info[j]))
	    break;
	if (j > i) {
	  
	  hb_glyph_info_t t = info[i];
	  memmove (&info[i], &info[i + 1], (j - i) * sizeof (info[0]));
	  info[j] = t;
	}
        break;
      }
  }

  
  if (!options.uniscribe_bug_compatible)
  {
    
    for (unsigned int i = start + 1; i < end; i++)
      if ((FLAG (info[i].indic_category()) & (FLAG (OT_ZWNJ) | FLAG (OT_ZWJ) | FLAG (OT_N) | FLAG (OT_H))))
	info[i].indic_position() = info[i - 1].indic_position();
  } else {
    



    
    for (unsigned int i = start + 1; i < end; i++)
      if ((FLAG (info[i].indic_category()) & (FLAG (OT_ZWNJ) | FLAG (OT_ZWJ) | FLAG (OT_N) | FLAG (OT_H)))) {
	info[i].indic_position() = info[i - 1].indic_position();
	if (info[i].indic_category() == OT_H && info[i].indic_position() == POS_PRE_M)
	  for (unsigned int j = i; j > start; j--)
	    if (info[j - 1].indic_position() != POS_PRE_M) {
	      info[i].indic_position() = info[j - 1].indic_position();
	      break;
	    }
      }
  }

  
  if (end - start < 64)
  {
    
    hb_bubble_sort (info + start, end - start, compare_indic_order);
    
    base = end;
    for (unsigned int i = start; i < end; i++)
      if (info[i].indic_position() == POS_BASE_C) {
        base = i;
	break;
      }
  }

  

  {
    hb_mask_t mask;

    
    for (unsigned int i = start; i < end && info[i].indic_position() == POS_RA_TO_BECOME_REPH; i++)
      info[i].mask |= mask_array[RPHF];

    
    mask = mask_array[HALF] | mask_array[AKHN] | mask_array[CJCT];
    for (unsigned int i = start; i < base; i++)
      info[i].mask  |= mask;
    
    mask = mask_array[AKHN] | mask_array[CJCT];
    info[base].mask |= mask;
    
    mask = mask_array[BLWF] | mask_array[PSTF] | mask_array[CJCT];
    for (unsigned int i = base + 1; i < end; i++)
      info[i].mask  |= mask;
  }

  
  for (unsigned int i = start + 1; i < end; i++)
    if (is_joiner (info[i])) {
      bool non_joiner = info[i].indic_category() == OT_ZWNJ;
      unsigned int j = i;

      do {
	j--;

	info[j].mask &= ~mask_array[CJCT];
	if (non_joiner)
	  info[j].mask &= ~mask_array[HALF];

      } while (j > start && !is_consonant (info[j]));
    }
}


static void
initial_reordering_vowel_syllable (const hb_ot_map_t *map,
				   hb_buffer_t *buffer,
				   hb_mask_t *mask_array,
				   unsigned int start, unsigned int end)
{
  
  initial_reordering_consonant_syllable (map, buffer, mask_array, start, end);
}

static void
initial_reordering_standalone_cluster (const hb_ot_map_t *map,
				       hb_buffer_t *buffer,
				       hb_mask_t *mask_array,
				       unsigned int start, unsigned int end)
{
  


  if (options.uniscribe_bug_compatible)
  {
    


    if (buffer->info[end - 1].indic_category() == OT_DOTTEDCIRCLE)
      return;
  }

  initial_reordering_consonant_syllable (map, buffer, mask_array, start, end);
}

static void
initial_reordering_non_indic (const hb_ot_map_t *map HB_UNUSED,
			      hb_buffer_t *buffer HB_UNUSED,
			      hb_mask_t *mask_array HB_UNUSED,
			      unsigned int start HB_UNUSED, unsigned int end HB_UNUSED)
{
  

}

#include "hb-ot-shape-complex-indic-machine.hh"

static void
initial_reordering (const hb_ot_map_t *map,
		    hb_face_t *face HB_UNUSED,
		    hb_buffer_t *buffer,
		    void *user_data HB_UNUSED)
{
  hb_mask_t mask_array[ARRAY_LENGTH (indic_basic_features)] = {0};
  unsigned int num_masks = ARRAY_LENGTH (indic_basic_features);
  for (unsigned int i = 0; i < num_masks; i++)
    mask_array[i] = map->get_1_mask (indic_basic_features[i].tag);

  find_syllables (map, buffer, mask_array);
}

static void
final_reordering_syllable (hb_buffer_t *buffer, hb_mask_t *mask_array,
			   unsigned int start, unsigned int end)
{
  hb_glyph_info_t *info = buffer->info;

  







  
  unsigned int base = end;
  for (unsigned int i = start; i < end; i++)
    if (info[i].indic_position() == POS_BASE_C) {
      base = i;
      break;
    }

  if (base == start) {
    

    buffer->merge_clusters (start, end);
    return;
  }

  unsigned int start_of_last_cluster = base;

  









  {
    unsigned int new_matra_pos = base - 1;
    while (new_matra_pos > start &&
	   !(FLAG (info[new_matra_pos].indic_category()) & (FLAG (OT_M) | FLAG (OT_H))))
      new_matra_pos--;
    

    if (info[new_matra_pos].indic_category() == OT_H &&
	info[new_matra_pos].indic_position() != POS_PRE_M) {
      
      if (new_matra_pos + 1 < end && is_joiner (info[new_matra_pos + 1]))
	new_matra_pos++;

      
      for (unsigned int i = new_matra_pos; i > start; i--)
	if (info[i - 1].indic_position () == POS_PRE_M)
	{
	  unsigned int old_matra_pos = i - 1;
	  hb_glyph_info_t matra = info[old_matra_pos];
	  memmove (&info[old_matra_pos], &info[old_matra_pos + 1], (new_matra_pos - old_matra_pos) * sizeof (info[0]));
	  info[new_matra_pos] = matra;
	  start_of_last_cluster = MIN (new_matra_pos, start_of_last_cluster);
	  new_matra_pos--;
	}
    }
  }


  








  


  if (start + 1 < end &&
      info[start].indic_position() == POS_RA_TO_BECOME_REPH &&
      info[start + 1].indic_position() != POS_RA_TO_BECOME_REPH)
  {
      unsigned int new_reph_pos;

     enum reph_position_t {
       REPH_AFTER_MAIN,
       REPH_BEFORE_SUBSCRIPT,
       REPH_AFTER_SUBSCRIPT,
       REPH_BEFORE_POSTSCRIPT,
       REPH_AFTER_POSTSCRIPT
     } reph_pos;

     
     switch ((hb_tag_t) buffer->props.script)
     {
       case HB_SCRIPT_MALAYALAM:
       case HB_SCRIPT_ORIYA:
	 reph_pos = REPH_AFTER_MAIN;
	 break;

       case HB_SCRIPT_GURMUKHI:
	 reph_pos = REPH_BEFORE_SUBSCRIPT;
	 break;

       case HB_SCRIPT_BENGALI:
	 reph_pos = REPH_AFTER_SUBSCRIPT;
	 break;

       default:
       case HB_SCRIPT_DEVANAGARI:
       case HB_SCRIPT_GUJARATI:
	 reph_pos = REPH_BEFORE_POSTSCRIPT;
	 break;

       case HB_SCRIPT_KANNADA:
       case HB_SCRIPT_TAMIL:
       case HB_SCRIPT_TELUGU:
	 reph_pos = REPH_AFTER_POSTSCRIPT;
	 break;
     }

    


    if (reph_pos == REPH_AFTER_POSTSCRIPT)
    {
      goto reph_step_5;
    }

    










    {
      new_reph_pos = start + 1;
      while (new_reph_pos < base && info[new_reph_pos].indic_category() != OT_H)
	new_reph_pos++;

      if (new_reph_pos < base && info[new_reph_pos].indic_category() == OT_H) {
	
	if (new_reph_pos + 1 < base && is_joiner (info[new_reph_pos + 1]))
	  new_reph_pos++;
	goto reph_move;
      }
    }

    



    if (reph_pos == REPH_AFTER_MAIN)
    {
      
    }

    




    
    if (reph_pos == REPH_AFTER_SUBSCRIPT)
    {
      new_reph_pos = base;
      while (new_reph_pos < end &&
	     !( FLAG (info[new_reph_pos + 1].indic_position()) & (FLAG (POS_POST_C) | FLAG (POS_POST_M) | FLAG (POS_SMVD))))
	new_reph_pos++;
      if (new_reph_pos < end)
        goto reph_move;
    }

    






    reph_step_5:
    {
      
    }

    

    {
      new_reph_pos = end - 1;
      while (new_reph_pos > start && info[new_reph_pos].indic_position() == POS_SMVD)
	new_reph_pos--;

      






      if (!options.uniscribe_bug_compatible &&
	  unlikely (info[new_reph_pos].indic_category() == OT_H)) {
	for (unsigned int i = base + 1; i < new_reph_pos; i++)
	  if (info[i].indic_category() == OT_M) {
	    
	    new_reph_pos--;
	  }
      }
      goto reph_move;
    }

    reph_move:
    {
      
      hb_glyph_info_t reph = info[start];
      memmove (&info[start], &info[start + 1], (new_reph_pos - start) * sizeof (info[0]));
      info[new_reph_pos] = reph;
      start_of_last_cluster = start; 
    }
  }


  















  



  
  if (info[start].indic_position () == POS_PRE_M &&
      (!start ||
       !(FLAG (_hb_glyph_info_get_general_category (&info[start - 1])) &
	 (FLAG (HB_UNICODE_GENERAL_CATEGORY_LOWERCASE_LETTER) |
	  FLAG (HB_UNICODE_GENERAL_CATEGORY_MODIFIER_LETTER) |
	  FLAG (HB_UNICODE_GENERAL_CATEGORY_OTHER_LETTER) |
	  FLAG (HB_UNICODE_GENERAL_CATEGORY_TITLECASE_LETTER) |
	  FLAG (HB_UNICODE_GENERAL_CATEGORY_UPPERCASE_LETTER) |
	  FLAG (HB_UNICODE_GENERAL_CATEGORY_SPACING_MARK) |
	  FLAG (HB_UNICODE_GENERAL_CATEGORY_ENCLOSING_MARK) |
	  FLAG (HB_UNICODE_GENERAL_CATEGORY_NON_SPACING_MARK)))))
    info[start].mask |= mask_array[INIT];



  

  if (!options.uniscribe_bug_compatible)
  {
    



    unsigned int cluster_start = start;
    for (unsigned int i = start + 1; i < start_of_last_cluster; i++)
      if (info[i - 1].indic_category() == OT_H && info[i].indic_category() == OT_ZWNJ) {
        i++;
	buffer->merge_clusters (cluster_start, i);
	cluster_start = i;
      }
    start_of_last_cluster = cluster_start;
  }

  buffer->merge_clusters (start_of_last_cluster, end);
}


static void
final_reordering (const hb_ot_map_t *map,
		  hb_face_t *face HB_UNUSED,
		  hb_buffer_t *buffer,
		  void *user_data HB_UNUSED)
{
  unsigned int count = buffer->len;
  if (!count) return;

  hb_mask_t mask_array[ARRAY_LENGTH (indic_other_features)] = {0};
  unsigned int num_masks = ARRAY_LENGTH (indic_other_features);
  for (unsigned int i = 0; i < num_masks; i++)
    mask_array[i] = map->get_1_mask (indic_other_features[i].tag);

  hb_glyph_info_t *info = buffer->info;
  unsigned int last = 0;
  unsigned int last_syllable = info[0].syllable();
  for (unsigned int i = 1; i < count; i++)
    if (last_syllable != info[i].syllable()) {
      final_reordering_syllable (buffer, mask_array, last, i);
      last = i;
      last_syllable = info[last].syllable();
    }
  final_reordering_syllable (buffer, mask_array, last, count);

  HB_BUFFER_DEALLOCATE_VAR (buffer, indic_category);
  HB_BUFFER_DEALLOCATE_VAR (buffer, indic_position);
}



