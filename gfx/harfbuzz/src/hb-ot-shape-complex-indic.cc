

























#include "hb-ot-shape-complex-indic-private.hh"
#include "hb-ot-layout-private.hh"






struct indic_options_t
{
  int initialized : 1;
  int uniscribe_bug_compatible : 1;
};

union indic_options_union_t {
  int i;
  indic_options_t opts;
};
ASSERT_STATIC (sizeof (int) == sizeof (indic_options_union_t));

static indic_options_union_t
indic_options_init (void)
{
  indic_options_union_t u;
  u.i = 0;
  u.opts.initialized = 1;

  char *c = getenv ("HB_OT_INDIC_OPTIONS");
  u.opts.uniscribe_bug_compatible = c && strstr (c, "uniscribe-bug-compatible");

  return u;
}

static inline indic_options_t
indic_options (void)
{
  static indic_options_union_t options;

  if (unlikely (!options.i)) {
    
    options = indic_options_init ();
  }

  return options.opts;
}










enum base_position_t {
  BASE_POS_FIRST,
  BASE_POS_LAST
};
enum reph_position_t {
  REPH_POS_DEFAULT     = POS_BEFORE_POST,

  REPH_POS_AFTER_MAIN  = POS_AFTER_MAIN,
  REPH_POS_BEFORE_SUB  = POS_BEFORE_SUB,
  REPH_POS_AFTER_SUB   = POS_AFTER_SUB,
  REPH_POS_BEFORE_POST = POS_BEFORE_POST,
  REPH_POS_AFTER_POST  = POS_AFTER_POST
};
enum reph_mode_t {
  REPH_MODE_IMPLICIT,  
  REPH_MODE_EXPLICIT,  
  REPH_MODE_VIS_REPHA, 
  REPH_MODE_LOG_REPHA  
};
struct indic_config_t
{
  hb_script_t     script;
  bool            has_old_spec;
  hb_codepoint_t  virama;
  base_position_t base_pos;
  reph_position_t reph_pos;
  reph_mode_t     reph_mode;
};

static const indic_config_t indic_configs[] =
{
  
  {HB_SCRIPT_INVALID,	false,     0,BASE_POS_LAST, REPH_POS_DEFAULT,    REPH_MODE_IMPLICIT},
  {HB_SCRIPT_DEVANAGARI,true, 0x094D,BASE_POS_LAST, REPH_POS_BEFORE_POST,REPH_MODE_IMPLICIT},
  {HB_SCRIPT_BENGALI,	true, 0x09CD,BASE_POS_LAST, REPH_POS_AFTER_SUB,  REPH_MODE_IMPLICIT},
  {HB_SCRIPT_GURMUKHI,	true, 0x0A4D,BASE_POS_LAST, REPH_POS_BEFORE_SUB, REPH_MODE_IMPLICIT},
  {HB_SCRIPT_GUJARATI,	true, 0x0ACD,BASE_POS_LAST, REPH_POS_BEFORE_POST,REPH_MODE_IMPLICIT},
  {HB_SCRIPT_ORIYA,	true, 0x0B4D,BASE_POS_LAST, REPH_POS_AFTER_MAIN, REPH_MODE_IMPLICIT},
  {HB_SCRIPT_TAMIL,	true, 0x0BCD,BASE_POS_LAST, REPH_POS_AFTER_POST, REPH_MODE_IMPLICIT},
  {HB_SCRIPT_TELUGU,	true, 0x0C4D,BASE_POS_LAST, REPH_POS_AFTER_POST, REPH_MODE_EXPLICIT},
  {HB_SCRIPT_KANNADA,	true, 0x0CCD,BASE_POS_LAST, REPH_POS_AFTER_POST, REPH_MODE_IMPLICIT},
  {HB_SCRIPT_MALAYALAM,	true, 0x0D4D,BASE_POS_LAST, REPH_POS_AFTER_MAIN, REPH_MODE_LOG_REPHA},
  {HB_SCRIPT_SINHALA,	false,0x0DCA,BASE_POS_FIRST,REPH_POS_AFTER_MAIN, REPH_MODE_EXPLICIT},
  {HB_SCRIPT_KHMER,	false,0x17D2,BASE_POS_FIRST,REPH_POS_DEFAULT,    REPH_MODE_VIS_REPHA},
};







struct feature_list_t {
  hb_tag_t tag;
  hb_bool_t is_global;
};

static const feature_list_t
indic_features[] =
{
  



  {HB_TAG('n','u','k','t'), true},
  {HB_TAG('a','k','h','n'), true},
  {HB_TAG('r','p','h','f'), false},
  {HB_TAG('r','k','r','f'), true},
  {HB_TAG('p','r','e','f'), false},
  {HB_TAG('h','a','l','f'), false},
  {HB_TAG('b','l','w','f'), false},
  {HB_TAG('a','b','v','f'), false},
  {HB_TAG('p','s','t','f'), false},
  {HB_TAG('c','f','a','r'), false},
  {HB_TAG('c','j','c','t'), true},
  {HB_TAG('v','a','t','u'), true},
  



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
  _NUKT,
  _AKHN,
  RPHF,
  _RKRF,
  PREF,
  HALF,
  BLWF,
  ABVF,
  PSTF,
  CFAR,
  _CJCT,
  _VATU,

  INIT,
  _PRES,
  _ABVS,
  _BLWS,
  _PSTS,
  _HALN,
  _DIST,
  _ABVM,
  _BLWM,

  INDIC_NUM_FEATURES,
  INDIC_BASIC_FEATURES = INIT 
};

static void
initial_reordering (const hb_ot_shape_plan_t *plan,
		    hb_font_t *font,
		    hb_buffer_t *buffer);
static void
final_reordering (const hb_ot_shape_plan_t *plan,
		  hb_font_t *font,
		  hb_buffer_t *buffer);

static void
collect_features_indic (hb_ot_shape_planner_t *plan)
{
  hb_ot_map_builder_t *map = &plan->map;

  map->add_bool_feature (HB_TAG('l','o','c','l'));
  

  map->add_bool_feature (HB_TAG('c','c','m','p'));


  unsigned int i = 0;
  map->add_gsub_pause (initial_reordering);
  for (; i < INDIC_BASIC_FEATURES; i++) {
    map->add_bool_feature (indic_features[i].tag, indic_features[i].is_global);
    map->add_gsub_pause (NULL);
  }
  map->add_gsub_pause (final_reordering);
  for (; i < INDIC_NUM_FEATURES; i++) {
    map->add_bool_feature (indic_features[i].tag, indic_features[i].is_global);
  }
}

static void
override_features_indic (hb_ot_shape_planner_t *plan)
{
  
  if (indic_options ().uniscribe_bug_compatible)
    plan->map.add_feature (HB_TAG('k','e','r','n'), 0, true);
}


struct would_substitute_feature_t
{
  inline void init (const hb_ot_map_t *map, hb_tag_t feature_tag)
  {
    map->get_stage_lookups (0,
			    map->get_feature_stage (0, feature_tag),
			    &lookups, &count);
  }

  inline bool would_substitute (hb_codepoint_t    *glyphs,
				unsigned int       glyphs_count,
				hb_face_t         *face) const
  {
    for (unsigned int i = 0; i < count; i++)
      if (hb_ot_layout_would_substitute_lookup_fast (face, glyphs, glyphs_count, lookups[i].index))
	return true;
    return false;
  }

  private:
  const hb_ot_map_t::lookup_map_t *lookups;
  unsigned int count;
};

struct indic_shape_plan_t
{
  ASSERT_POD ();

  inline bool get_virama_glyph (hb_font_t *font, hb_codepoint_t *pglyph) const
  {
    hb_codepoint_t glyph = virama_glyph;
    if (unlikely (virama_glyph == (hb_codepoint_t) -1))
    {
      if (!config->virama || !font->get_glyph (config->virama, 0, &glyph))
	glyph = 0;
      


      

      (const_cast<indic_shape_plan_t *> (this))->virama_glyph = glyph;
    }

    *pglyph = glyph;
    return glyph != 0;
  }

  const indic_config_t *config;

  bool is_old_spec;
  hb_codepoint_t virama_glyph;

  would_substitute_feature_t pref;
  would_substitute_feature_t blwf;
  would_substitute_feature_t pstf;

  hb_mask_t mask_array[INDIC_NUM_FEATURES];
};

static void *
data_create_indic (const hb_ot_shape_plan_t *plan)
{
  indic_shape_plan_t *indic_plan = (indic_shape_plan_t *) calloc (1, sizeof (indic_shape_plan_t));
  if (unlikely (!indic_plan))
    return NULL;

  indic_plan->config = &indic_configs[0];
  for (unsigned int i = 1; i < ARRAY_LENGTH (indic_configs); i++)
    if (plan->props.script == indic_configs[i].script) {
      indic_plan->config = &indic_configs[i];
      break;
    }

  indic_plan->is_old_spec = indic_plan->config->has_old_spec && ((plan->map.get_chosen_script (0) & 0x000000FF) != '2');
  indic_plan->virama_glyph = (hb_codepoint_t) -1;

  indic_plan->pref.init (&plan->map, HB_TAG('p','r','e','f'));
  indic_plan->blwf.init (&plan->map, HB_TAG('b','l','w','f'));
  indic_plan->pstf.init (&plan->map, HB_TAG('p','s','t','f'));

  for (unsigned int i = 0; i < ARRAY_LENGTH (indic_plan->mask_array); i++)
    indic_plan->mask_array[i] = indic_features[i].is_global ? 0 : plan->map.get_1_mask (indic_features[i].tag);

  return indic_plan;
}

static void
data_destroy_indic (void *data)
{
  free (data);
}

static indic_position_t
consonant_position_from_face (const indic_shape_plan_t *indic_plan,
			      hb_codepoint_t *glyphs, unsigned int glyphs_len,
			      hb_face_t      *face)
{
  if (indic_plan->pref.would_substitute (glyphs, glyphs_len, face)) return POS_BELOW_C;
  if (indic_plan->blwf.would_substitute (glyphs, glyphs_len, face)) return POS_BELOW_C;
  if (indic_plan->pstf.would_substitute (glyphs, glyphs_len, face)) return POS_POST_C;
  return POS_BASE_C;
}


static void
setup_masks_indic (const hb_ot_shape_plan_t *plan HB_UNUSED,
		   hb_buffer_t              *buffer,
		   hb_font_t                *font HB_UNUSED)
{
  HB_BUFFER_ALLOCATE_VAR (buffer, indic_category);
  HB_BUFFER_ALLOCATE_VAR (buffer, indic_position);

  


  unsigned int count = buffer->len;
  for (unsigned int i = 0; i < count; i++)
    set_indic_properties (buffer->info[i]);
}

static int
compare_indic_order (const hb_glyph_info_t *pa, const hb_glyph_info_t *pb)
{
  int a = pa->indic_position();
  int b = pb->indic_position();

  return a < b ? -1 : a == b ? 0 : +1;
}



static void
update_consonant_positions (const hb_ot_shape_plan_t *plan,
			    hb_font_t         *font,
			    hb_buffer_t       *buffer)
{
  const indic_shape_plan_t *indic_plan = (const indic_shape_plan_t *) plan->data;

  unsigned int consonant_pos = indic_plan->is_old_spec ? 0 : 1;
  hb_codepoint_t glyphs[2];
  if (indic_plan->get_virama_glyph (font, &glyphs[1 - consonant_pos]))
  {
    hb_face_t *face = font->face;
    unsigned int count = buffer->len;
    for (unsigned int i = 0; i < count; i++)
      if (buffer->info[i].indic_position() == POS_BASE_C) {
	glyphs[consonant_pos] = buffer->info[i].codepoint;
	buffer->info[i].indic_position() = consonant_position_from_face (indic_plan, glyphs, 2, face);
      }
  }
}





static void
initial_reordering_consonant_syllable (const hb_ot_shape_plan_t *plan, hb_buffer_t *buffer,
				       unsigned int start, unsigned int end)
{
  const indic_shape_plan_t *indic_plan = (const indic_shape_plan_t *) plan->data;
  hb_glyph_info_t *info = buffer->info;


  













  unsigned int base = end;
  bool has_reph = false;

  {
    


    unsigned int limit = start;
    if (indic_plan->mask_array[RPHF] &&
	start + 3 <= end &&
	info[start].indic_category() == OT_Ra &&
	info[start + 1].indic_category() == OT_H &&
	(
	 (indic_plan->config->reph_mode == REPH_MODE_IMPLICIT && !is_joiner (info[start + 2])) ||
	 (indic_plan->config->reph_mode == REPH_MODE_EXPLICIT && info[start + 2].indic_category() == OT_ZWJ)
	))
    {
      limit += 2;
      while (limit < end && is_joiner (info[limit]))
        limit++;
      base = start;
      has_reph = true;
    };

    switch (indic_plan->config->base_pos == BASE_POS_LAST)
    {
      case BASE_POS_LAST:
      {
	
	unsigned int i = end;
	bool seen_below = false;
	do {
	  i--;
	  
	  if (is_consonant (info[i]))
	  {
	    

	    if (info[i].indic_position() != POS_BELOW_C &&
		(info[i].indic_position() != POS_POST_C || seen_below))
	    {
	      base = i;
	      break;
	    }
	    if (info[i].indic_position() == POS_BELOW_C)
	      seen_below = true;

	    







	    

	    base = i;
	  }
	  else
	  {
	    




	    if (start < i &&
		info[i].indic_category() == OT_ZWJ &&
		info[i - 1].indic_category() == OT_H)
	      break;
	  }
	} while (i > limit);
      }
      break;

      case BASE_POS_FIRST:
      {
	

	if (!has_reph)
	  base = limit;

	

	for (unsigned int i = limit; i < end; i++)
	  if (is_consonant (info[i]) && info[i].indic_position() == POS_BASE_C)
	  {
	    if (limit < i && info[i - 1].indic_category() == OT_ZWJ)
	      break;
	    else
	      base = i;
	  }

	
	for (unsigned int i = base + 1; i < end; i++)
	  if (is_consonant (info[i]) && info[i].indic_position() == POS_BASE_C)
	    info[i].indic_position() = POS_BELOW_C;
      }
      break;

      default:
      abort ();
    }

    




    if (has_reph && base == start && start + 2 == limit) {
      
      has_reph = false;
    }
  }

  if (base < end)
    info[base].indic_position() = POS_BASE_C;


  




















  










  

  for (unsigned int i = start; i < base; i++)
    info[i].indic_position() = MIN (POS_PRE_C, (indic_position_t) info[i].indic_position());

  if (base < end)
    info[base].indic_position() = POS_BASE_C;

  

  for (unsigned int i = base + 1; i < end; i++)
    if (info[i].indic_category() == OT_M) {
      for (unsigned int j = i + 1; j < end; j++)
        if (is_consonant (info[j])) {
	  info[j].indic_position() = POS_FINAL_C;
	  break;
	}
      break;
    }

  
  if (has_reph)
    info[start].indic_position() = POS_RA_TO_BECOME_REPH;

  

  if (indic_plan->is_old_spec) {
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

  
  {
    indic_position_t last_pos = POS_START;
    for (unsigned int i = start; i < end; i++)
    {
      if ((FLAG (info[i].indic_category()) & (JOINER_FLAGS | FLAG (OT_N) | FLAG (OT_RS) | HALANT_OR_COENG_FLAGS)))
      {
	info[i].indic_position() = last_pos;
	if (unlikely (indic_options ().uniscribe_bug_compatible &&
		      info[i].indic_category() == OT_H &&
		      info[i].indic_position() == POS_PRE_M))
	{
	  



	  for (unsigned int j = i; j > start; j--)
	    if (info[j - 1].indic_position() != POS_PRE_M) {
	      info[i].indic_position() = info[j - 1].indic_position();
	      break;
	    }
	}
      } else if (info[i].indic_position() != POS_SMVD) {
        last_pos = (indic_position_t) info[i].indic_position();
      }
    }
  }
  
  {
    unsigned int last_halant = end;
    for (unsigned int i = base + 1; i < end; i++)
      if (is_halant_or_coeng (info[i]))
        last_halant = i;
      else if (is_consonant (info[i])) {
	for (unsigned int j = last_halant; j < i; j++)
	  if (info[j].indic_position() != POS_SMVD)
	    info[j].indic_position() = info[i].indic_position();
      }
  }

  {
    


    buffer->merge_clusters (base, end);
    
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
      info[i].mask |= indic_plan->mask_array[RPHF];

    
    mask = indic_plan->mask_array[HALF];
    for (unsigned int i = start; i < base; i++)
      info[i].mask  |= mask;
    
    mask = 0;
    if (base < end)
      info[base].mask |= mask;
    
    mask = indic_plan->mask_array[BLWF] | indic_plan->mask_array[ABVF] | indic_plan->mask_array[PSTF];
    for (unsigned int i = base + 1; i < end; i++)
      info[i].mask  |= mask;
  }

  
  if (indic_plan->mask_array[PREF] && base + 2 < end)
  {
    
    for (unsigned int i = base + 1; i + 1 < end; i++)
      if (is_halant_or_coeng (info[i]) &&
	  info[i + 1].indic_category() == OT_Ra)
      {
	info[i++].mask |= indic_plan->mask_array[PREF];
	info[i++].mask |= indic_plan->mask_array[PREF];

	





	for (; i < end; i++)
	  info[i].mask |= indic_plan->mask_array[CFAR];

	break;
      }
  }

  
  for (unsigned int i = start + 1; i < end; i++)
    if (is_joiner (info[i])) {
      bool non_joiner = info[i].indic_category() == OT_ZWNJ;
      unsigned int j = i;

      do {
	j--;

	


	
	if (non_joiner)
	  info[j].mask &= ~indic_plan->mask_array[HALF];

      } while (j > start && !is_consonant (info[j]));
    }
}


static void
initial_reordering_vowel_syllable (const hb_ot_shape_plan_t *plan,
				   hb_buffer_t *buffer,
				   unsigned int start, unsigned int end)
{
  
  initial_reordering_consonant_syllable (plan, buffer, start, end);
}

static void
initial_reordering_standalone_cluster (const hb_ot_shape_plan_t *plan,
				       hb_buffer_t *buffer,
				       unsigned int start, unsigned int end)
{
  


  if (indic_options ().uniscribe_bug_compatible)
  {
    


    if (buffer->info[end - 1].indic_category() == OT_DOTTEDCIRCLE)
      return;
  }

  initial_reordering_consonant_syllable (plan, buffer, start, end);
}

static void
initial_reordering_non_indic (const hb_ot_shape_plan_t *plan HB_UNUSED,
			      hb_buffer_t *buffer HB_UNUSED,
			      unsigned int start HB_UNUSED, unsigned int end HB_UNUSED)
{
  

}

#include "hb-ot-shape-complex-indic-machine.hh"

static void
initial_reordering (const hb_ot_shape_plan_t *plan,
		    hb_font_t *font,
		    hb_buffer_t *buffer)
{
  update_consonant_positions (plan, font, buffer);
  find_syllables (plan, buffer);
}

static void
final_reordering_syllable (const hb_ot_shape_plan_t *plan,
			   hb_buffer_t *buffer,
			   unsigned int start, unsigned int end)
{
  const indic_shape_plan_t *indic_plan = (const indic_shape_plan_t *) plan->data;
  hb_glyph_info_t *info = buffer->info;

  







  
  unsigned int base;
  for (base = start; base < end; base++)
    if (info[base].indic_position() >= POS_BASE_C) {
      if (start < base && info[base].indic_position() > POS_BASE_C)
        base--;
      break;
    }


  









  if (start + 1 < end && start < base) 
  {
    
    unsigned int new_pos = base == end ? base - 2 : base - 1;

    



    if (buffer->props.script != HB_SCRIPT_MALAYALAM)
    {
      while (new_pos > start &&
	     !(is_one_of (info[new_pos], (FLAG (OT_M) | FLAG (OT_H) | FLAG (OT_Coeng)))))
	new_pos--;

      


      if (is_halant_or_coeng (info[new_pos]) &&
	  info[new_pos].indic_position() != POS_PRE_M)
      {
	
	if (new_pos + 1 < end && is_joiner (info[new_pos + 1]))
	  new_pos++;
      }
      else
        new_pos = start; 
    }

    if (start < new_pos)
    {
      
      for (unsigned int i = new_pos; i > start; i--)
	if (info[i - 1].indic_position () == POS_PRE_M)
	{
	  unsigned int old_pos = i - 1;
	  hb_glyph_info_t tmp = info[old_pos];
	  memmove (&info[old_pos], &info[old_pos + 1], (new_pos - old_pos) * sizeof (info[0]));
	  info[new_pos] = tmp;
	  new_pos--;
	}
      buffer->merge_clusters (new_pos, MIN (end, base + 1));
    } else {
      for (unsigned int i = start; i < base; i++)
	if (info[i].indic_position () == POS_PRE_M) {
	  buffer->merge_clusters (i, MIN (end, base + 1));
	  break;
	}
    }
  }


  








  


  if (start + 1 < end &&
      info[start].indic_position() == POS_RA_TO_BECOME_REPH &&
      info[start + 1].indic_position() != POS_RA_TO_BECOME_REPH)
  {
    unsigned int new_reph_pos;
    reph_position_t reph_pos = indic_plan->config->reph_pos;

    

    


    if (reph_pos == REPH_POS_AFTER_POST)
    {
      goto reph_step_5;
    }

    










    {
      new_reph_pos = start + 1;
      while (new_reph_pos < base && !is_halant_or_coeng (info[new_reph_pos]))
	new_reph_pos++;

      if (new_reph_pos < base && is_halant_or_coeng (info[new_reph_pos])) {
	
	if (new_reph_pos + 1 < base && is_joiner (info[new_reph_pos + 1]))
	  new_reph_pos++;
	goto reph_move;
      }
    }

    



    if (reph_pos == REPH_POS_AFTER_MAIN)
    {
      new_reph_pos = base;
      
      while (new_reph_pos + 1 < end && info[new_reph_pos + 1].indic_position() <= POS_AFTER_MAIN)
	new_reph_pos++;
      if (new_reph_pos < end)
        goto reph_move;
    }

    




    
    if (reph_pos == REPH_POS_AFTER_SUB)
    {
      new_reph_pos = base;
      while (new_reph_pos < end &&
	     !( FLAG (info[new_reph_pos + 1].indic_position()) & (FLAG (POS_POST_C) | FLAG (POS_AFTER_POST) | FLAG (POS_SMVD))))
	new_reph_pos++;
      if (new_reph_pos < end)
        goto reph_move;
    }

    






    reph_step_5:
    {
      
      new_reph_pos = start + 1;
      while (new_reph_pos < base && !is_halant_or_coeng (info[new_reph_pos]))
	new_reph_pos++;

      if (new_reph_pos < base && is_halant_or_coeng (info[new_reph_pos])) {
	
	if (new_reph_pos + 1 < base && is_joiner (info[new_reph_pos + 1]))
	  new_reph_pos++;
	goto reph_move;
      }
    }

    

    {
      new_reph_pos = end - 1;
      while (new_reph_pos > start && info[new_reph_pos].indic_position() == POS_SMVD)
	new_reph_pos--;

      






      if (!indic_options ().uniscribe_bug_compatible &&
	  unlikely (is_halant_or_coeng (info[new_reph_pos]))) {
	for (unsigned int i = base + 1; i < new_reph_pos; i++)
	  if (info[i].indic_category() == OT_M) {
	    
	    new_reph_pos--;
	  }
      }
      goto reph_move;
    }

    reph_move:
    {
      
      buffer->merge_clusters (start, end);

      
      hb_glyph_info_t reph = info[start];
      memmove (&info[start], &info[start + 1], (new_reph_pos - start) * sizeof (info[0]));
      info[new_reph_pos] = reph;
    }
  }


  





  if (indic_plan->mask_array[PREF] && base + 1 < end) 
  {
    for (unsigned int i = base + 1; i < end; i++)
      if ((info[i].mask & indic_plan->mask_array[PREF]) != 0)
      {
	



	if (i + 1 == end || (info[i + 1].mask & indic_plan->mask_array[PREF]) == 0)
	{
	  







	  unsigned int new_pos = base;
	  while (new_pos > start &&
		 !(is_one_of (info[new_pos - 1], FLAG(OT_M) | HALANT_OR_COENG_FLAGS)))
	    new_pos--;

	  

	  if (new_pos > start && info[new_pos - 1].indic_category() == OT_M)
	  {
	    unsigned int old_pos = i;
	    for (unsigned int i = base + 1; i < old_pos; i++)
	      if (info[i].indic_category() == OT_M)
	      {
		new_pos--;
		break;
	      }
	  }

	  if (new_pos > start && is_halant_or_coeng (info[new_pos - 1]))
	    
	    if (new_pos < end && is_joiner (info[new_pos]))
	      new_pos++;

	  {
	    unsigned int old_pos = i;
	    buffer->merge_clusters (new_pos, old_pos + 1);
	    hb_glyph_info_t tmp = info[old_pos];
	    memmove (&info[new_pos + 1], &info[new_pos], (old_pos - new_pos) * sizeof (info[0]));
	    info[new_pos] = tmp;
	  }
	}

        break;
      }
  }


  
  if (info[start].indic_position () == POS_PRE_M &&
      (!start ||
       !(FLAG (_hb_glyph_info_get_general_category (&info[start - 1])) &
	 FLAG_RANGE (HB_UNICODE_GENERAL_CATEGORY_FORMAT, HB_UNICODE_GENERAL_CATEGORY_NON_SPACING_MARK))))
    info[start].mask |= indic_plan->mask_array[INIT];


  


  if (indic_options ().uniscribe_bug_compatible)
  {
    



    buffer->merge_clusters (start, end);
  }
}


static void
final_reordering (const hb_ot_shape_plan_t *plan,
		  hb_font_t *font,
		  hb_buffer_t *buffer)
{
  unsigned int count = buffer->len;
  if (!count) return;

  hb_glyph_info_t *info = buffer->info;
  unsigned int last = 0;
  unsigned int last_syllable = info[0].syllable();
  for (unsigned int i = 1; i < count; i++)
    if (last_syllable != info[i].syllable()) {
      final_reordering_syllable (plan, buffer, last, i);
      last = i;
      last_syllable = info[last].syllable();
    }
  final_reordering_syllable (plan, buffer, last, count);

  HB_BUFFER_DEALLOCATE_VAR (buffer, indic_category);
  HB_BUFFER_DEALLOCATE_VAR (buffer, indic_position);
}


const hb_ot_complex_shaper_t _hb_ot_complex_shaper_indic =
{
  "indic",
  collect_features_indic,
  override_features_indic,
  data_create_indic,
  data_destroy_indic,
  NULL, 
  NULL, 
  setup_masks_indic,
  false, 
};
