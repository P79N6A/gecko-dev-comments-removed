

























#include "hb-ot-shape-complex-indic-private.hh"
#include "hb-ot-layout-private.hh"


#define indic_category() complex_var_u8_0() /* indic_category_t */
#define indic_position() complex_var_u8_1() /* indic_position_t */







#define IN_HALF_BLOCK(u, Base) (((u) & ~0x7F) == (Base))

#define IS_DEVA(u) (IN_HALF_BLOCK (u, 0x0900))
#define IS_BENG(u) (IN_HALF_BLOCK (u, 0x0980))
#define IS_GURU(u) (IN_HALF_BLOCK (u, 0x0A00))
#define IS_GUJR(u) (IN_HALF_BLOCK (u, 0x0A80))
#define IS_ORYA(u) (IN_HALF_BLOCK (u, 0x0B00))
#define IS_TAML(u) (IN_HALF_BLOCK (u, 0x0B80))
#define IS_TELU(u) (IN_HALF_BLOCK (u, 0x0C00))
#define IS_KNDA(u) (IN_HALF_BLOCK (u, 0x0C80))
#define IS_MLYM(u) (IN_HALF_BLOCK (u, 0x0D00))
#define IS_SINH(u) (IN_HALF_BLOCK (u, 0x0D80))
#define IS_KHMR(u) (IN_HALF_BLOCK (u, 0x1780))


#define MATRA_POS_LEFT(u)	POS_PRE_M
#define MATRA_POS_RIGHT(u)	( \
				  IS_DEVA(u) ? POS_AFTER_SUB  : \
				  IS_BENG(u) ? POS_AFTER_POST : \
				  IS_GURU(u) ? POS_AFTER_POST : \
				  IS_GUJR(u) ? POS_AFTER_POST : \
				  IS_ORYA(u) ? POS_AFTER_POST : \
				  IS_TAML(u) ? POS_AFTER_POST : \
				  IS_TELU(u) ? (u <= 0x0C42 ? POS_BEFORE_SUB : POS_AFTER_SUB) : \
				  IS_KNDA(u) ? (u < 0x0CC3 || u > 0xCD6 ? POS_BEFORE_SUB : POS_AFTER_SUB) : \
				  IS_MLYM(u) ? POS_AFTER_POST : \
				  IS_SINH(u) ? POS_AFTER_SUB  : \
				  IS_KHMR(u) ? POS_AFTER_POST : \
				  /*default*/  POS_AFTER_SUB    \
				)
#define MATRA_POS_TOP(u)	( /* BENG and MLYM don't have top matras. */ \
				  IS_DEVA(u) ? POS_AFTER_SUB  : \
				  IS_GURU(u) ? POS_AFTER_POST : /* Deviate from spec */ \
				  IS_GUJR(u) ? POS_AFTER_SUB  : \
				  IS_ORYA(u) ? POS_AFTER_MAIN : \
				  IS_TAML(u) ? POS_AFTER_SUB  : \
				  IS_TELU(u) ? POS_BEFORE_SUB : \
				  IS_KNDA(u) ? POS_BEFORE_SUB : \
				  IS_SINH(u) ? POS_AFTER_SUB  : \
				  IS_KHMR(u) ? POS_AFTER_POST : \
				  /*default*/  POS_AFTER_SUB    \
				)
#define MATRA_POS_BOTTOM(u)	( \
				  IS_DEVA(u) ? POS_AFTER_SUB  : \
				  IS_BENG(u) ? POS_AFTER_SUB  : \
				  IS_GURU(u) ? POS_AFTER_POST : \
				  IS_GUJR(u) ? POS_AFTER_POST : \
				  IS_ORYA(u) ? POS_AFTER_SUB  : \
				  IS_TAML(u) ? POS_AFTER_POST : \
				  IS_TELU(u) ? POS_BEFORE_SUB : \
				  IS_KNDA(u) ? POS_BEFORE_SUB : \
				  IS_MLYM(u) ? POS_AFTER_POST : \
				  IS_SINH(u) ? POS_AFTER_SUB  : \
				  IS_KHMR(u) ? POS_AFTER_POST : \
				  /*default*/  POS_AFTER_SUB    \
				)

static inline indic_position_t
matra_position (hb_codepoint_t u, indic_position_t side)
{
  switch ((int) side)
  {
    case POS_PRE_C:	return MATRA_POS_LEFT (u);
    case POS_POST_C:	return MATRA_POS_RIGHT (u);
    case POS_ABOVE_C:	return MATRA_POS_TOP (u);
    case POS_BELOW_C:	return MATRA_POS_BOTTOM (u);
  };
  return side;
}





static const hb_codepoint_t ra_chars[] = {
  0x0930, 
  0x09B0, 
  0x09F0, 
  0x0A30, 	
  0x0AB0, 
  0x0B30, 
  0x0BB0, 		
  0x0C30, 		
  0x0CB0, 
  0x0D30, 	

  0x0DBB, 		

  0x179A, 		
};

static inline indic_position_t
consonant_position (hb_codepoint_t  u)
{
  if ((u & ~0x007F) == 0x1780)
    return POS_BELOW_C; 
  return POS_BASE_C; 
}

static inline bool
is_ra (hb_codepoint_t u)
{
  for (unsigned int i = 0; i < ARRAY_LENGTH (ra_chars); i++)
    if (u == ra_chars[i])
      return true;
  return false;
}

static inline bool
is_one_of (const hb_glyph_info_t &info, unsigned int flags)
{
  
  if (is_a_ligature (info)) return false;
  return !!(FLAG (info.indic_category()) & flags);
}

#define JOINER_FLAGS (FLAG (OT_ZWJ) | FLAG (OT_ZWNJ))
static inline bool
is_joiner (const hb_glyph_info_t &info)
{
  return is_one_of (info, JOINER_FLAGS);
}






#define CONSONANT_FLAGS (FLAG (OT_C) | FLAG (OT_CM) | FLAG (OT_Ra) | FLAG (OT_V) | FLAG (OT_NBSP) | FLAG (OT_DOTTEDCIRCLE))
static inline bool
is_consonant (const hb_glyph_info_t &info)
{
  return is_one_of (info, CONSONANT_FLAGS);
}

#define HALANT_OR_COENG_FLAGS (FLAG (OT_H) | FLAG (OT_Coeng))
static inline bool
is_halant_or_coeng (const hb_glyph_info_t &info)
{
  return is_one_of (info, HALANT_OR_COENG_FLAGS);
}

static inline void
set_indic_properties (hb_glyph_info_t &info)
{
  hb_codepoint_t u = info.codepoint;
  unsigned int type = hb_indic_get_categories (u);
  indic_category_t cat = (indic_category_t) (type & 0x7F);
  indic_position_t pos = (indic_position_t) (type >> 8);


  




  







  if (unlikely (hb_in_range<hb_codepoint_t> (u, 0x0951, 0x0954)))
    cat = OT_VD;

  if (unlikely (u == 0x17D1))
    cat = OT_X;
  if (cat == OT_X &&
      unlikely (hb_in_range<hb_codepoint_t> (u, 0x17CB, 0x17D3))) 
  {
    
    cat = OT_M;
    pos = POS_ABOVE_C;
  }
  if (u == 0x17C6) 
    cat = OT_N;

  if (unlikely (u == 0x17D2)) cat = OT_Coeng; 
  else if (unlikely (u == 0x200C)) cat = OT_ZWNJ;
  else if (unlikely (u == 0x200D)) cat = OT_ZWJ;
  else if (unlikely (u == 0x25CC)) cat = OT_DOTTEDCIRCLE;
  else if (unlikely (u == 0x0A71)) cat = OT_SM; 

  if (cat == OT_Repha) {
    





    if (_hb_glyph_info_get_general_category (&info) == HB_UNICODE_GENERAL_CATEGORY_NON_SPACING_MARK)
      cat = OT_N;
  }



  



  if ((FLAG (cat) & CONSONANT_FLAGS))
  {
    pos = consonant_position (u);
    if (is_ra (u))
      cat = OT_Ra;
  }
  else if (cat == OT_M)
  {
    pos = matra_position (u, pos);
  }
  else if (cat == OT_SM || cat == OT_VD)
  {
    pos = POS_SMVD;
  }

  if (unlikely (u == 0x0B01)) pos = POS_BEFORE_SUB; 



  info.indic_category() = cat;
  info.indic_position() = pos;
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
  {HB_TAG('b','l','w','f'), false},
  {HB_TAG('h','a','l','f'), false},
  {HB_TAG('a','b','v','f'), false},
  {HB_TAG('p','s','t','f'), false},
  {HB_TAG('c','f','a','r'), false},
  {HB_TAG('v','a','t','u'), true},
  {HB_TAG('c','j','c','t'), true},
  



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
  BLWF,
  HALF,
  ABVF,
  PSTF,
  CFAR,
  _VATU,
  _CJCT,

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
setup_syllables (const hb_ot_shape_plan_t *plan,
		 hb_font_t *font,
		 hb_buffer_t *buffer);
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

  
  map->add_gsub_pause (setup_syllables);

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
  
  if (hb_options ().uniscribe_bug_compatible)
    plan->map.add_feature (HB_TAG('k','e','r','n'), 0, true);

  plan->map.add_feature (HB_TAG('l','i','g','a'), 0, true);
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
				bool               zero_context,
				hb_face_t         *face) const
  {
    for (unsigned int i = 0; i < count; i++)
      if (hb_ot_layout_lookup_would_substitute_fast (face, lookups[i].index, glyphs, glyphs_count, zero_context))
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

  would_substitute_feature_t rphf;
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

  indic_plan->is_old_spec = indic_plan->config->has_old_spec && ((plan->map.chosen_script[0] & 0x000000FF) != '2');
  indic_plan->virama_glyph = (hb_codepoint_t) -1;

  indic_plan->rphf.init (&plan->map, HB_TAG('r','p','h','f'));
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
  bool zero_context = indic_plan->is_old_spec ? false : true;
  if (indic_plan->pref.would_substitute (glyphs, glyphs_len, zero_context, face)) return POS_BELOW_C;
  if (indic_plan->blwf.would_substitute (glyphs, glyphs_len, zero_context, face)) return POS_BELOW_C;
  if (indic_plan->pstf.would_substitute (glyphs, glyphs_len, zero_context, face)) return POS_POST_C;
  return POS_BASE_C;
}


enum syllable_type_t {
  consonant_syllable,
  vowel_syllable,
  standalone_cluster,
  broken_cluster,
  non_indic_cluster,
};

#include "hb-ot-shape-complex-indic-machine.hh"


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

static void
setup_syllables (const hb_ot_shape_plan_t *plan HB_UNUSED,
		 hb_font_t *font HB_UNUSED,
		 hb_buffer_t *buffer)
{
  find_syllables (buffer);
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
initial_reordering_consonant_syllable (const hb_ot_shape_plan_t *plan,
				       hb_face_t *face,
				       hb_buffer_t *buffer,
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
	(
	 (indic_plan->config->reph_mode == REPH_MODE_IMPLICIT && !is_joiner (info[start + 2])) ||
	 (indic_plan->config->reph_mode == REPH_MODE_EXPLICIT && info[start + 2].indic_category() == OT_ZWJ)
	))
    {
      
      hb_codepoint_t glyphs[2] = {info[start].codepoint, info[start + 1].codepoint};
      if (indic_plan->rphf.would_substitute (glyphs, ARRAY_LENGTH (glyphs), true, face))
      {
	limit += 2;
	while (limit < end && is_joiner (info[limit]))
	  limit++;
	base = start;
	has_reph = true;
      }
    } else if (indic_plan->config->reph_mode == REPH_MODE_LOG_REPHA && info[start].indic_category() == OT_Repha)
    {
	limit += 1;
	while (limit < end && is_joiner (info[limit]))
	  limit++;
	base = start;
	has_reph = true;
    }

    switch (indic_plan->config->base_pos)
    {
      default:
        assert (false);
	

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
    }

    




    if (has_reph && base == start && limit - base <= 2) {
      
      has_reph = false;
    }
  }


  




















  










  

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
	  if (is_consonant (info[j]) || info[j].indic_category() == OT_H)
	    break;
	if (info[j].indic_category() != OT_H && j > i) {
	  
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
	if (unlikely (info[i].indic_category() == OT_H &&
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

  if (indic_plan->is_old_spec &&
      buffer->props.script == HB_SCRIPT_DEVANAGARI)
  {
    

















    for (unsigned int i = start; i + 1 < base; i++)
      if (info[i  ].indic_category() == OT_Ra &&
	  info[i+1].indic_category() == OT_H  &&
	  (i + 2 == base ||
	   info[i+2].indic_category() != OT_ZWJ))
      {
	info[i  ].mask |= indic_plan->mask_array[BLWF];
	info[i+1].mask |= indic_plan->mask_array[BLWF];
      }
  }

  if (indic_plan->mask_array[PREF] && base + 2 < end)
  {
    
    for (unsigned int i = base + 1; i + 1 < end; i++) {
      hb_codepoint_t glyphs[2] = {info[i].codepoint, info[i + 1].codepoint};
      if (indic_plan->pref.would_substitute (glyphs, ARRAY_LENGTH (glyphs), true, face))
      {
	info[i++].mask |= indic_plan->mask_array[PREF];
	info[i++].mask |= indic_plan->mask_array[PREF];

	





	for (; i < end; i++)
	  info[i].mask |= indic_plan->mask_array[CFAR];

	break;
      }
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
				   hb_face_t *face,
				   hb_buffer_t *buffer,
				   unsigned int start, unsigned int end)
{
  
  initial_reordering_consonant_syllable (plan, face, buffer, start, end);
}

static void
initial_reordering_standalone_cluster (const hb_ot_shape_plan_t *plan,
				       hb_face_t *face,
				       hb_buffer_t *buffer,
				       unsigned int start, unsigned int end)
{
  


  if (hb_options ().uniscribe_bug_compatible)
  {
    


    if (buffer->info[end - 1].indic_category() == OT_DOTTEDCIRCLE)
      return;
  }

  initial_reordering_consonant_syllable (plan, face, buffer, start, end);
}

static void
initial_reordering_broken_cluster (const hb_ot_shape_plan_t *plan,
				   hb_face_t *face,
				   hb_buffer_t *buffer,
				   unsigned int start, unsigned int end)
{
  
  initial_reordering_standalone_cluster (plan, face, buffer, start, end);
}

static void
initial_reordering_non_indic_cluster (const hb_ot_shape_plan_t *plan HB_UNUSED,
				      hb_face_t *face HB_UNUSED,
				      hb_buffer_t *buffer HB_UNUSED,
				      unsigned int start HB_UNUSED, unsigned int end HB_UNUSED)
{
  

}


static void
initial_reordering_syllable (const hb_ot_shape_plan_t *plan,
			     hb_face_t *face,
			     hb_buffer_t *buffer,
			     unsigned int start, unsigned int end)
{
  syllable_type_t syllable_type = (syllable_type_t) (buffer->info[start].syllable() & 0x0F);
  switch (syllable_type) {
  case consonant_syllable:	initial_reordering_consonant_syllable (plan, face, buffer, start, end); return;
  case vowel_syllable:		initial_reordering_vowel_syllable     (plan, face, buffer, start, end); return;
  case standalone_cluster:	initial_reordering_standalone_cluster (plan, face, buffer, start, end); return;
  case broken_cluster:		initial_reordering_broken_cluster     (plan, face, buffer, start, end); return;
  case non_indic_cluster:	initial_reordering_non_indic_cluster  (plan, face, buffer, start, end); return;
  }
}

static inline void
insert_dotted_circles (const hb_ot_shape_plan_t *plan HB_UNUSED,
		       hb_font_t *font,
		       hb_buffer_t *buffer)
{
  
  bool has_broken_syllables = false;
  unsigned int count = buffer->len;
  for (unsigned int i = 0; i < count; i++)
    if ((buffer->info[i].syllable() & 0x0F) == broken_cluster) {
      has_broken_syllables = true;
      break;
    }
  if (likely (!has_broken_syllables))
    return;


  hb_codepoint_t dottedcircle_glyph;
  if (!font->get_glyph (0x25CC, 0, &dottedcircle_glyph))
    return;

  hb_glyph_info_t dottedcircle = {0};
  dottedcircle.codepoint = 0x25CC;
  set_indic_properties (dottedcircle);
  dottedcircle.codepoint = dottedcircle_glyph;

  buffer->clear_output ();

  buffer->idx = 0;
  unsigned int last_syllable = 0;
  while (buffer->idx < buffer->len)
  {
    unsigned int syllable = buffer->cur().syllable();
    syllable_type_t syllable_type = (syllable_type_t) (syllable & 0x0F);
    if (unlikely (last_syllable != syllable && syllable_type == broken_cluster))
    {
      last_syllable = syllable;

      hb_glyph_info_t info = dottedcircle;
      info.cluster = buffer->cur().cluster;
      info.mask = buffer->cur().mask;
      info.syllable() = buffer->cur().syllable();

      
      while (buffer->idx < buffer->len &&
	     last_syllable == buffer->cur().syllable() &&
	     buffer->cur().indic_category() == OT_Repha)
        buffer->next_glyph ();

      buffer->output_info (info);
    }
    else
      buffer->next_glyph ();
  }

  buffer->swap_buffers ();
}

static void
initial_reordering (const hb_ot_shape_plan_t *plan,
		    hb_font_t *font,
		    hb_buffer_t *buffer)
{
  update_consonant_positions (plan, font, buffer);
  insert_dotted_circles (plan, font, buffer);

  hb_glyph_info_t *info = buffer->info;
  unsigned int count = buffer->len;
  if (unlikely (!count)) return;
  unsigned int last = 0;
  unsigned int last_syllable = info[0].syllable();
  for (unsigned int i = 1; i < count; i++)
    if (last_syllable != info[i].syllable()) {
      initial_reordering_syllable (plan, font->face, buffer, last, i);
      last = i;
      last_syllable = info[last].syllable();
    }
  initial_reordering_syllable (plan, font->face, buffer, last, count);
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
  if (base == end && start < base &&
      info[base - 1].indic_category() != OT_ZWJ)
    base--;
  while (start < base &&
	 (info[base].indic_category() == OT_H ||
	  info[base].indic_category() == OT_N))
    base--;


  









  if (start + 1 < end && start < base) 
  {
    
    unsigned int new_pos = base == end ? base - 2 : base - 1;

    



    if (buffer->props.script != HB_SCRIPT_MALAYALAM && buffer->props.script != HB_SCRIPT_TAMIL)
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

    if (start < new_pos && info[new_pos].indic_position () != POS_PRE_M)
    {
      
      for (unsigned int i = new_pos; i > start; i--)
	if (info[i - 1].indic_position () == POS_PRE_M)
	{
	  unsigned int old_pos = i - 1;
	  hb_glyph_info_t tmp = info[old_pos];
	  memmove (&info[old_pos], &info[old_pos + 1], (new_pos - old_pos) * sizeof (info[0]));
	  info[new_pos] = tmp;
	  if (old_pos < base && base <= new_pos) 
	    base--;
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

      






      if (!hb_options ().uniscribe_bug_compatible &&
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
      if (start < base && base <= new_reph_pos)
	base--;
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
	  



	  if (buffer->props.script != HB_SCRIPT_MALAYALAM && buffer->props.script != HB_SCRIPT_TAMIL)
	  {
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
	    if (new_pos <= base && base < old_pos)
	      base++;
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


  


  if (hb_options ().uniscribe_bug_compatible)
  {
    



    buffer->merge_clusters (start, end);
  }
}


static void
final_reordering (const hb_ot_shape_plan_t *plan,
		  hb_font_t *font HB_UNUSED,
		  hb_buffer_t *buffer)
{
  unsigned int count = buffer->len;
  if (unlikely (!count)) return;

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

  
  for (unsigned int i = 0; i < count; i++)
    info[i].syllable() = 0;

  HB_BUFFER_DEALLOCATE_VAR (buffer, indic_category);
  HB_BUFFER_DEALLOCATE_VAR (buffer, indic_position);
}


static hb_ot_shape_normalization_mode_t
normalization_preference_indic (const hb_segment_properties_t *props HB_UNUSED)
{
  return HB_OT_SHAPE_NORMALIZATION_MODE_COMPOSED_DIACRITICS_NO_SHORT_CIRCUIT;
}

static bool
decompose_indic (const hb_ot_shape_normalize_context_t *c,
		 hb_codepoint_t  ab,
		 hb_codepoint_t *a,
		 hb_codepoint_t *b)
{
  switch (ab)
  {
    
    case 0x0931  : return false;
    case 0x0B94  : return false;


    



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
    
    
    case 0x0B57  : *a = no decomp, -> RIGHT; return true;
    case 0x1C29  : *a = no decomp, -> LEFT; return true;
    case 0xA9C0  : *a = no decomp, -> RIGHT; return true;
    case 0x111BF  : *a = no decomp, -> ABOVE; return true;
#endif
  }

  if ((ab == 0x0DDA || hb_in_range<hb_codepoint_t> (ab, 0x0DDC, 0x0DDE)))
  {
    

























    const indic_shape_plan_t *indic_plan = (const indic_shape_plan_t *) c->plan->data;

    hb_codepoint_t glyph;

    if (hb_options ().uniscribe_bug_compatible ||
	(c->font->get_glyph (ab, 0, &glyph) &&
	 indic_plan->pstf.would_substitute (&glyph, 1, true, c->font->face)))
    {
      
      *a = 0x0DD9;
      *b = ab;
      return true;
    }
  }

  return c->unicode->decompose (ab, a, b);
}

static bool
compose_indic (const hb_ot_shape_normalize_context_t *c,
	       hb_codepoint_t  a,
	       hb_codepoint_t  b,
	       hb_codepoint_t *ab)
{
  
  if (HB_UNICODE_GENERAL_CATEGORY_IS_MARK (c->unicode->general_category (a)))
    return false;

  
  if (a == 0x09AF && b == 0x09BC) { *ab = 0x09DF; return true; }

  return c->unicode->compose (a, b, ab);
}


const hb_ot_complex_shaper_t _hb_ot_complex_shaper_indic =
{
  "indic",
  collect_features_indic,
  override_features_indic,
  data_create_indic,
  data_destroy_indic,
  NULL, 
  normalization_preference_indic,
  decompose_indic,
  compose_indic,
  setup_masks_indic,
  HB_OT_SHAPE_ZERO_WIDTH_MARKS_NONE,
  false, 
};
