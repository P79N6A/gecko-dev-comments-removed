

























#include "hb-ot-shape-complex-indic-private.hh"
#include "hb-ot-layout-private.hh"


#define indic_category() complex_var_u8_0() /* indic_category_t */
#define indic_position() complex_var_u8_1() /* indic_position_t */







#define IN_HALF_BLOCK(u, Base) (((u) & ~0x7Fu) == (Base))

#define IS_DEVA(u) (IN_HALF_BLOCK (u, 0x0900u))
#define IS_BENG(u) (IN_HALF_BLOCK (u, 0x0980u))
#define IS_GURU(u) (IN_HALF_BLOCK (u, 0x0A00u))
#define IS_GUJR(u) (IN_HALF_BLOCK (u, 0x0A80u))
#define IS_ORYA(u) (IN_HALF_BLOCK (u, 0x0B00u))
#define IS_TAML(u) (IN_HALF_BLOCK (u, 0x0B80u))
#define IS_TELU(u) (IN_HALF_BLOCK (u, 0x0C00u))
#define IS_KNDA(u) (IN_HALF_BLOCK (u, 0x0C80u))
#define IS_MLYM(u) (IN_HALF_BLOCK (u, 0x0D00u))
#define IS_SINH(u) (IN_HALF_BLOCK (u, 0x0D80u))
#define IS_KHMR(u) (IN_HALF_BLOCK (u, 0x1780u))


#define MATRA_POS_LEFT(u)	POS_PRE_M
#define MATRA_POS_RIGHT(u)	( \
				  IS_DEVA(u) ? POS_AFTER_SUB  : \
				  IS_BENG(u) ? POS_AFTER_POST : \
				  IS_GURU(u) ? POS_AFTER_POST : \
				  IS_GUJR(u) ? POS_AFTER_POST : \
				  IS_ORYA(u) ? POS_AFTER_POST : \
				  IS_TAML(u) ? POS_AFTER_POST : \
				  IS_TELU(u) ? (u <= 0x0C42u ? POS_BEFORE_SUB : POS_AFTER_SUB) : \
				  IS_KNDA(u) ? (u < 0x0CC3u || u > 0xCD6u ? POS_BEFORE_SUB : POS_AFTER_SUB) : \
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
  0x0930u, 
  0x09B0u, 
  0x09F0u, 
  0x0A30u, 	
  0x0AB0u, 
  0x0B30u, 
  0x0BB0u, 		
  0x0C30u, 		
  0x0CB0u, 
  0x0D30u, 	

  0x0DBBu, 		

  0x179Au, 		
};

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
  
  if (_hb_glyph_info_ligated (&info)) return false;
  return !!(FLAG (info.indic_category()) & flags);
}

static inline bool
is_joiner (const hb_glyph_info_t &info)
{
  return is_one_of (info, JOINER_FLAGS);
}

static inline bool
is_consonant (const hb_glyph_info_t &info)
{
  return is_one_of (info, CONSONANT_FLAGS);
}

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
  indic_category_t cat = (indic_category_t) (type & 0x7Fu);
  indic_position_t pos = (indic_position_t) (type >> 8);


  




  









  if (unlikely (hb_in_ranges (u, 0x0951u, 0x0952u,
				 0x1CD0u, 0x1CD2u,
				 0x1CD4u, 0x1CE1u) ||
			    u == 0x1CF4u))
    cat = OT_A;
  
  else if (unlikely (hb_in_range (u, 0x0953u, 0x0954u)))
    cat = OT_SM;
  
  else if (unlikely (hb_in_ranges (u, 0x0A72u, 0x0A73u,
				      0x1CF5u, 0x1CF6u)))
    cat = OT_C;
  

  else if (unlikely (hb_in_range (u, 0x1CE2u, 0x1CE8u)))
    cat = OT_A;
  


  else if (unlikely (u == 0x1CEDu))
    cat = OT_A;
  
  else if (unlikely (hb_in_ranges (u, 0xA8F2u, 0xA8F7u,
				      0x1CE9u, 0x1CECu,
				      0x1CEEu, 0x1CF1u)))
  {
    cat = OT_Symbol;
    ASSERT_STATIC ((int) INDIC_SYLLABIC_CATEGORY_AVAGRAHA == OT_Symbol);
  }
  else if (unlikely (hb_in_range (u, 0x17CDu, 0x17D1u) ||
		     u == 0x17CBu || u == 0x17D3u || u == 0x17DDu)) 
  {
    
    cat = OT_M;
    pos = POS_ABOVE_C;
  }
  else if (unlikely (u == 0x17C6u)) cat = OT_N; 
  else if (unlikely (u == 0x17D2u)) cat = OT_Coeng; 
  else if (unlikely (hb_in_range (u, 0x2010u, 0x2011u)))
				    cat = OT_PLACEHOLDER;
  else if (unlikely (u == 0x25CCu)) cat = OT_DOTTEDCIRCLE;
  else if (unlikely (u == 0xA982u)) cat = OT_SM; 
  else if (unlikely (u == 0xA9BEu)) cat = OT_CM2; 
  else if (unlikely (u == 0xA9BDu)) { cat = OT_M; pos = POS_POST_C; } 


  



  if ((FLAG (cat) & CONSONANT_FLAGS))
  {
    pos = POS_BASE_C;
    if (is_ra (u))
      cat = OT_Ra;
  }
  else if (cat == OT_M)
  {
    pos = matra_position (u, pos);
  }
  else if ((FLAG (cat) & (FLAG (OT_SM) | FLAG (OT_VD) | FLAG (OT_A) | FLAG (OT_Symbol))))
  {
    pos = POS_SMVD;
  }

  if (unlikely (u == 0x0B01u)) pos = POS_BEFORE_SUB; 



  info.indic_category() = cat;
  info.indic_position() = pos;
}














enum base_position_t {
  BASE_POS_FIRST,
  BASE_POS_LAST_SINHALA,
  BASE_POS_LAST
};
enum reph_position_t {
  REPH_POS_AFTER_MAIN  = POS_AFTER_MAIN,
  REPH_POS_BEFORE_SUB  = POS_BEFORE_SUB,
  REPH_POS_AFTER_SUB   = POS_AFTER_SUB,
  REPH_POS_BEFORE_POST = POS_BEFORE_POST,
  REPH_POS_AFTER_POST  = POS_AFTER_POST,
  REPH_POS_DONT_CARE   = POS_RA_TO_BECOME_REPH
};
enum reph_mode_t {
  REPH_MODE_IMPLICIT,  
  REPH_MODE_EXPLICIT,  
  REPH_MODE_VIS_REPHA, 
  REPH_MODE_LOG_REPHA  
};
enum blwf_mode_t {
  BLWF_MODE_PRE_AND_POST, 
  BLWF_MODE_POST_ONLY     
};
enum pref_len_t {
  PREF_LEN_1 = 1,
  PREF_LEN_2 = 2,
  PREF_LEN_DONT_CARE = PREF_LEN_2
};
struct indic_config_t
{
  hb_script_t     script;
  bool            has_old_spec;
  hb_codepoint_t  virama;
  base_position_t base_pos;
  reph_position_t reph_pos;
  reph_mode_t     reph_mode;
  blwf_mode_t     blwf_mode;
  pref_len_t      pref_len;
};

static const indic_config_t indic_configs[] =
{
  
  {HB_SCRIPT_INVALID,	false,      0,BASE_POS_LAST, REPH_POS_BEFORE_POST,REPH_MODE_IMPLICIT, BLWF_MODE_PRE_AND_POST, PREF_LEN_1},
  {HB_SCRIPT_DEVANAGARI,true, 0x094Du,BASE_POS_LAST, REPH_POS_BEFORE_POST,REPH_MODE_IMPLICIT, BLWF_MODE_PRE_AND_POST, PREF_LEN_DONT_CARE},
  {HB_SCRIPT_BENGALI,	true, 0x09CDu,BASE_POS_LAST, REPH_POS_AFTER_SUB,  REPH_MODE_IMPLICIT, BLWF_MODE_PRE_AND_POST, PREF_LEN_DONT_CARE},
  {HB_SCRIPT_GURMUKHI,	true, 0x0A4Du,BASE_POS_LAST, REPH_POS_BEFORE_SUB, REPH_MODE_IMPLICIT, BLWF_MODE_PRE_AND_POST, PREF_LEN_DONT_CARE},
  {HB_SCRIPT_GUJARATI,	true, 0x0ACDu,BASE_POS_LAST, REPH_POS_BEFORE_POST,REPH_MODE_IMPLICIT, BLWF_MODE_PRE_AND_POST, PREF_LEN_DONT_CARE},
  {HB_SCRIPT_ORIYA,	true, 0x0B4Du,BASE_POS_LAST, REPH_POS_AFTER_MAIN, REPH_MODE_IMPLICIT, BLWF_MODE_PRE_AND_POST, PREF_LEN_DONT_CARE},
  {HB_SCRIPT_TAMIL,	true, 0x0BCDu,BASE_POS_LAST, REPH_POS_AFTER_POST, REPH_MODE_IMPLICIT, BLWF_MODE_PRE_AND_POST, PREF_LEN_2},
  {HB_SCRIPT_TELUGU,	true, 0x0C4Du,BASE_POS_LAST, REPH_POS_AFTER_POST, REPH_MODE_EXPLICIT, BLWF_MODE_POST_ONLY,    PREF_LEN_2},
  {HB_SCRIPT_KANNADA,	true, 0x0CCDu,BASE_POS_LAST, REPH_POS_AFTER_POST, REPH_MODE_IMPLICIT, BLWF_MODE_POST_ONLY,    PREF_LEN_2},
  {HB_SCRIPT_MALAYALAM,	true, 0x0D4Du,BASE_POS_LAST, REPH_POS_AFTER_MAIN, REPH_MODE_LOG_REPHA,BLWF_MODE_PRE_AND_POST, PREF_LEN_2},
  {HB_SCRIPT_SINHALA,	false,0x0DCAu,BASE_POS_LAST_SINHALA,
						     REPH_POS_AFTER_MAIN, REPH_MODE_EXPLICIT, BLWF_MODE_PRE_AND_POST, PREF_LEN_DONT_CARE},
  {HB_SCRIPT_KHMER,	false,0x17D2u,BASE_POS_FIRST,REPH_POS_DONT_CARE,  REPH_MODE_VIS_REPHA,BLWF_MODE_PRE_AND_POST, PREF_LEN_2},
  {HB_SCRIPT_JAVANESE,	false,0xA9C0u,BASE_POS_FIRST,REPH_POS_DONT_CARE,  REPH_MODE_VIS_REPHA,BLWF_MODE_PRE_AND_POST, PREF_LEN_1},
};







struct feature_list_t {
  hb_tag_t tag;
  hb_ot_map_feature_flags_t flags;
};

static const feature_list_t
indic_features[] =
{
  



  {HB_TAG('n','u','k','t'), F_GLOBAL},
  {HB_TAG('a','k','h','n'), F_GLOBAL},
  {HB_TAG('r','p','h','f'), F_NONE},
  {HB_TAG('r','k','r','f'), F_GLOBAL},
  {HB_TAG('p','r','e','f'), F_NONE},
  {HB_TAG('b','l','w','f'), F_NONE},
  {HB_TAG('a','b','v','f'), F_NONE},
  {HB_TAG('h','a','l','f'), F_NONE},
  {HB_TAG('p','s','t','f'), F_NONE},
  {HB_TAG('v','a','t','u'), F_GLOBAL},
  {HB_TAG('c','j','c','t'), F_GLOBAL},
  {HB_TAG('c','f','a','r'), F_NONE},
  





  {HB_TAG('i','n','i','t'), F_NONE},
  {HB_TAG('p','r','e','s'), F_GLOBAL},
  {HB_TAG('a','b','v','s'), F_GLOBAL},
  {HB_TAG('b','l','w','s'), F_GLOBAL},
  {HB_TAG('p','s','t','s'), F_GLOBAL},
  {HB_TAG('h','a','l','n'), F_GLOBAL},
  
  {HB_TAG('d','i','s','t'), F_GLOBAL},
  {HB_TAG('a','b','v','m'), F_GLOBAL},
  {HB_TAG('b','l','w','m'), F_GLOBAL},
};




enum {
  _NUKT,
  _AKHN,
  RPHF,
  _RKRF,
  PREF,
  BLWF,
  ABVF,
  HALF,
  PSTF,
  _VATU,
  _CJCT,
  CFAR,

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
clear_syllables (const hb_ot_shape_plan_t *plan,
		 hb_font_t *font,
		 hb_buffer_t *buffer);

static void
collect_features_indic (hb_ot_shape_planner_t *plan)
{
  hb_ot_map_builder_t *map = &plan->map;

  
  map->add_gsub_pause (setup_syllables);

  map->add_global_bool_feature (HB_TAG('l','o','c','l'));
  

  map->add_global_bool_feature (HB_TAG('c','c','m','p'));


  unsigned int i = 0;
  map->add_gsub_pause (initial_reordering);
  for (; i < INDIC_BASIC_FEATURES; i++) {
    map->add_feature (indic_features[i].tag, 1, indic_features[i].flags | F_MANUAL_ZWJ);
    map->add_gsub_pause (NULL);
  }
  map->add_gsub_pause (final_reordering);
  for (; i < INDIC_NUM_FEATURES; i++) {
    map->add_feature (indic_features[i].tag, 1, indic_features[i].flags | F_MANUAL_ZWJ);
  }

  map->add_global_bool_feature (HB_TAG('c','a','l','t'));
  map->add_global_bool_feature (HB_TAG('c','l','i','g'));

  map->add_gsub_pause (clear_syllables);
}

static void
override_features_indic (hb_ot_shape_planner_t *plan)
{
  
  if (hb_options ().uniscribe_bug_compatible)
  {
    switch ((hb_tag_t) plan->props.script)
    {
      case HB_SCRIPT_KHMER:
	plan->map.add_feature (HB_TAG('k','e','r','n'), 0, F_GLOBAL);
	break;
    }
  }

  plan->map.add_feature (HB_TAG('l','i','g','a'), 0, F_GLOBAL);
}


struct would_substitute_feature_t
{
  inline void init (const hb_ot_map_t *map, hb_tag_t feature_tag, bool zero_context_)
  {
    zero_context = zero_context_;
    map->get_stage_lookups (0,
			    map->get_feature_stage (0, feature_tag),
			    &lookups, &count);
  }

  inline bool would_substitute (const hb_codepoint_t *glyphs,
				unsigned int          glyphs_count,
				hb_face_t            *face) const
  {
    for (unsigned int i = 0; i < count; i++)
      if (hb_ot_layout_lookup_would_substitute_fast (face, lookups[i].index, glyphs, glyphs_count, zero_context))
	return true;
    return false;
  }

  private:
  const hb_ot_map_t::lookup_map_t *lookups;
  unsigned int count;
  bool zero_context;
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

  indic_plan->is_old_spec = indic_plan->config->has_old_spec && ((plan->map.chosen_script[0] & 0x000000FFu) != '2');
  indic_plan->virama_glyph = (hb_codepoint_t) -1;

  

  bool zero_context = !indic_plan->is_old_spec;
  indic_plan->rphf.init (&plan->map, HB_TAG('r','p','h','f'), zero_context);
  indic_plan->pref.init (&plan->map, HB_TAG('p','r','e','f'), zero_context);
  indic_plan->blwf.init (&plan->map, HB_TAG('b','l','w','f'), zero_context);
  indic_plan->pstf.init (&plan->map, HB_TAG('p','s','t','f'), zero_context);

  for (unsigned int i = 0; i < ARRAY_LENGTH (indic_plan->mask_array); i++)
    indic_plan->mask_array[i] = (indic_features[i].flags & F_GLOBAL) ?
				 0 : plan->map.get_1_mask (indic_features[i].tag);

  return indic_plan;
}

static void
data_destroy_indic (void *data)
{
  free (data);
}

static indic_position_t
consonant_position_from_face (const indic_shape_plan_t *indic_plan,
			      const hb_codepoint_t consonant,
			      const hb_codepoint_t virama,
			      hb_face_t *face)
{
  









  hb_codepoint_t glyphs[3] = {virama, consonant, virama};
  if (indic_plan->blwf.would_substitute (glyphs  , 2, face) ||
      indic_plan->blwf.would_substitute (glyphs+1, 2, face))
    return POS_BELOW_C;
  if (indic_plan->pstf.would_substitute (glyphs  , 2, face) ||
      indic_plan->pstf.would_substitute (glyphs+1, 2, face))
    return POS_POST_C;
  unsigned int pref_len = indic_plan->config->pref_len;
  if ((pref_len == PREF_LEN_2 &&
       (indic_plan->pref.would_substitute (glyphs  , 2, face) ||
        indic_plan->pref.would_substitute (glyphs+1, 2, face)))
   || (pref_len == PREF_LEN_1 &&
       indic_plan->pref.would_substitute (glyphs+1, 1, face)))
    return POS_POST_C;
  return POS_BASE_C;
}


enum syllable_type_t {
  consonant_syllable,
  vowel_syllable,
  standalone_cluster,
  symbol_cluster,
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
  hb_glyph_info_t *info = buffer->info;
  for (unsigned int i = 0; i < count; i++)
    set_indic_properties (info[i]);
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

  if (indic_plan->config->base_pos != BASE_POS_LAST)
    return;

  hb_codepoint_t virama;
  if (indic_plan->get_virama_glyph (font, &virama))
  {
    hb_face_t *face = font->face;
    unsigned int count = buffer->len;
    hb_glyph_info_t *info = buffer->info;
    for (unsigned int i = 0; i < count; i++)
      if (info[i].indic_position() == POS_BASE_C)
      {
	hb_codepoint_t consonant = info[i].codepoint;
	info[i].indic_position() = consonant_position_from_face (indic_plan, consonant, virama, face);
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
    if (indic_plan->config->reph_pos != REPH_POS_DONT_CARE &&
	indic_plan->mask_array[RPHF] &&
	start + 3 <= end &&
	(
	 (indic_plan->config->reph_mode == REPH_MODE_IMPLICIT && !is_joiner (info[start + 2])) ||
	 (indic_plan->config->reph_mode == REPH_MODE_EXPLICIT && info[start + 2].indic_category() == OT_ZWJ)
	))
    {
      
      hb_codepoint_t glyphs[3] = {info[start].codepoint,
				  info[start + 1].codepoint,
				  indic_plan->config->reph_mode == REPH_MODE_EXPLICIT ?
				    info[start + 2].codepoint : 0};
      if (indic_plan->rphf.would_substitute (glyphs, 2, face) ||
	  (indic_plan->config->reph_mode == REPH_MODE_EXPLICIT &&
	   indic_plan->rphf.would_substitute (glyphs, 3, face)))
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

      case BASE_POS_LAST_SINHALA:
      {
        




	if (!has_reph)
	  base = limit;

	

	for (unsigned int i = limit; i < end; i++)
	  if (is_consonant (info[i]))
	  {
	    if (limit < i && info[i - 1].indic_category() == OT_ZWJ)
	      break;
	    else
	      base = i;
	  }

	
	for (unsigned int i = base + 1; i < end; i++)
	  if (is_consonant (info[i]))
	    info[i].indic_position() = POS_BELOW_C;
      }
      break;

      case BASE_POS_FIRST:
      {
	

	assert (indic_plan->config->reph_mode == REPH_MODE_VIS_REPHA);
	assert (!has_reph);

	base = start;

	
	for (unsigned int i = base + 1; i < end; i++)
	  if (is_consonant (info[i]))
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

  
















  if (indic_plan->is_old_spec)
  {
    bool disallow_double_halants = buffer->props.script != HB_SCRIPT_MALAYALAM;
    for (unsigned int i = base + 1; i < end; i++)
      if (info[i].indic_category() == OT_H)
      {
        unsigned int j;
        for (j = end - 1; j > i; j--)
	  if (is_consonant (info[j]) ||
	      (disallow_double_halants && info[j].indic_category() == OT_H))
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
      if ((FLAG (info[i].indic_category()) & (JOINER_FLAGS | FLAG (OT_N) | FLAG (OT_RS) | MEDIAL_FLAGS | HALANT_OR_COENG_FLAGS)))
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
    unsigned int last = base;
    for (unsigned int i = base + 1; i < end; i++)
      if (is_consonant (info[i]))
      {
	for (unsigned int j = last + 1; j < i; j++)
	  if (info[j].indic_position() < POS_SMVD)
	    info[j].indic_position() = info[i].indic_position();
	last = i;
      } else if (info[i].indic_category() == OT_M)
        last = i;
  }


  {
    
    unsigned int syllable = info[start].syllable();
    for (unsigned int i = start; i < end; i++)
      info[i].syllable() = i - start;

    
    hb_bubble_sort (info + start, end - start, compare_indic_order);
    
    base = end;
    for (unsigned int i = start; i < end; i++)
      if (info[i].indic_position() == POS_BASE_C)
      {
	base = i;
	break;
      }
    




    if (indic_plan->is_old_spec || end - base > 127)
      buffer->merge_clusters (base, end);
    else
    {
      
      for (unsigned int i = base; i < end; i++)
        if (info[i].syllable() != 255)
	{
	  unsigned int max = i;
	  unsigned int j = start + info[i].syllable();
	  while (j != i)
	  {
	    max = MAX (max, j);
	    unsigned int next = start + info[j].syllable();
	    info[j].syllable() = 255; 
	    j = next;
	  }
	  if (i != max)
	    buffer->merge_clusters (i, max + 1);
	}
    }

    
    for (unsigned int i = start; i < end; i++)
      info[i].syllable() = syllable;
  }

  

  {
    hb_mask_t mask;

    
    for (unsigned int i = start; i < end && info[i].indic_position() == POS_RA_TO_BECOME_REPH; i++)
      info[i].mask |= indic_plan->mask_array[RPHF];

    
    mask = indic_plan->mask_array[HALF];
    if (!indic_plan->is_old_spec &&
	indic_plan->config->blwf_mode == BLWF_MODE_PRE_AND_POST)
      mask |= indic_plan->mask_array[BLWF];
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

  unsigned int pref_len = indic_plan->config->pref_len;
  if (indic_plan->mask_array[PREF] && base + pref_len < end)
  {
    assert (1 <= pref_len && pref_len <= 2);
    
    for (unsigned int i = base + 1; i + pref_len - 1 < end; i++) {
      hb_codepoint_t glyphs[2];
      for (unsigned int j = 0; j < pref_len; j++)
        glyphs[j] = info[i + j].codepoint;
      if (indic_plan->pref.would_substitute (glyphs, pref_len, face))
      {
	for (unsigned int j = 0; j < pref_len; j++)
	  info[i++].mask |= indic_plan->mask_array[PREF];

	





	if (indic_plan->mask_array[CFAR])
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
initial_reordering_symbol_cluster (const hb_ot_shape_plan_t *plan HB_UNUSED,
				   hb_face_t *face HB_UNUSED,
				   hb_buffer_t *buffer HB_UNUSED,
				   unsigned int start HB_UNUSED, unsigned int end HB_UNUSED)
{
  

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
  case symbol_cluster:		initial_reordering_symbol_cluster     (plan, face, buffer, start, end); return;
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
  hb_glyph_info_t *info = buffer->info;
  for (unsigned int i = 0; i < count; i++)
    if ((info[i].syllable() & 0x0F) == broken_cluster)
    {
      has_broken_syllables = true;
      break;
    }
  if (likely (!has_broken_syllables))
    return;


  hb_codepoint_t dottedcircle_glyph;
  if (!font->get_glyph (0x25CCu, 0, &dottedcircle_glyph))
    return;

  hb_glyph_info_t dottedcircle = {0};
  dottedcircle.codepoint = 0x25CCu;
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


  




  if (indic_plan->virama_glyph)
  {
    unsigned int virama_glyph = indic_plan->virama_glyph;
    for (unsigned int i = start; i < end; i++)
      if (info[i].codepoint == virama_glyph &&
	  _hb_glyph_info_ligated (&info[i]) &&
	  _hb_glyph_info_multiplied (&info[i]))
      {
        
	info[i].indic_category() = OT_H;
	_hb_glyph_info_clear_ligated_and_multiplied (&info[i]);
      }
  }


  







  bool try_pref = !!indic_plan->mask_array[PREF];

  
  unsigned int base;
  for (base = start; base < end; base++)
    if (info[base].indic_position() >= POS_BASE_C)
    {
      if (try_pref && base + 1 < end && indic_plan->config->pref_len == 2)
      {
	for (unsigned int i = base + 1; i < end; i++)
	  if ((info[i].mask & indic_plan->mask_array[PREF]) != 0)
	  {
	    if (!(_hb_glyph_info_substituted (&info[i]) &&
		  _hb_glyph_info_ligated_and_didnt_multiply (&info[i])))
	    {
	      

	      base = i;
	      while (base < end && is_halant_or_coeng (info[base]))
		base++;
	      info[base].indic_position() = POS_BASE_C;

	      try_pref = false;
	    }
	    break;
	  }
      }

      if (start < base && info[base].indic_position() > POS_BASE_C)
        base--;
      break;
    }
  if (base == end && start < base &&
      is_one_of (info[base - 1], FLAG (OT_ZWJ)))
    base--;
  if (base < end)
    while (start < base &&
	   is_one_of (info[base], (FLAG (OT_N) | HALANT_OR_COENG_FLAGS)))
      base--;


  









  if (start + 1 < end && start < base) 
  {
    
    unsigned int new_pos = base == end ? base - 2 : base - 1;

    



    if (buffer->props.script != HB_SCRIPT_MALAYALAM && buffer->props.script != HB_SCRIPT_TAMIL)
    {
      while (new_pos > start &&
	     !(is_one_of (info[new_pos], (FLAG (OT_M) | HALANT_OR_COENG_FLAGS))))
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
	  buffer->merge_clusters (new_pos, MIN (end, base + 1));
	  new_pos--;
	}
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
      ((info[start].indic_category() == OT_Repha) ^
       _hb_glyph_info_ligated_and_didnt_multiply (&info[start])))
  {
    unsigned int new_reph_pos;
    reph_position_t reph_pos = indic_plan->config->reph_pos;

    assert (reph_pos != REPH_POS_DONT_CARE);

    


    if (reph_pos == REPH_POS_AFTER_POST)
    {
      goto reph_step_5;
    }

    










    {
      new_reph_pos = start + 1;
      while (new_reph_pos < base && !is_halant_or_coeng (info[new_reph_pos]))
	new_reph_pos++;

      if (new_reph_pos < base && is_halant_or_coeng (info[new_reph_pos]))
      {
	
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

      if (new_reph_pos < base && is_halant_or_coeng (info[new_reph_pos]))
      {
	
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
      buffer->merge_clusters (start, new_reph_pos + 1);

      
      hb_glyph_info_t reph = info[start];
      memmove (&info[start], &info[start + 1], (new_reph_pos - start) * sizeof (info[0]));
      info[new_reph_pos] = reph;
      if (start < base && base <= new_reph_pos)
	base--;
    }
  }


  





  if (try_pref && base + 1 < end) 
  {
    unsigned int pref_len = indic_plan->config->pref_len;
    for (unsigned int i = base + 1; i < end; i++)
      if ((info[i].mask & indic_plan->mask_array[PREF]) != 0)
      {
	



        




	if (_hb_glyph_info_substituted (&info[i]) &&
	    ((pref_len == 1) ^ _hb_glyph_info_ligated_and_didnt_multiply (&info[i])))
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
	  {
	    
	    if (new_pos < end && is_joiner (info[new_pos]))
	      new_pos++;
	  }

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
    switch ((hb_tag_t) plan->props.script)
    {
      case HB_SCRIPT_TAMIL:
      case HB_SCRIPT_SINHALA:
        break;

      default:
	



	buffer->merge_clusters (start, end);
	break;
    }
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

  HB_BUFFER_DEALLOCATE_VAR (buffer, indic_category);
  HB_BUFFER_DEALLOCATE_VAR (buffer, indic_position);
}


static void
clear_syllables (const hb_ot_shape_plan_t *plan HB_UNUSED,
		 hb_font_t *font HB_UNUSED,
		 hb_buffer_t *buffer)
{
  hb_glyph_info_t *info = buffer->info;
  unsigned int count = buffer->len;
  for (unsigned int i = 0; i < count; i++)
    info[i].syllable() = 0;
}


static bool
decompose_indic (const hb_ot_shape_normalize_context_t *c,
		 hb_codepoint_t  ab,
		 hb_codepoint_t *a,
		 hb_codepoint_t *b)
{
  switch (ab)
  {
    
    case 0x0931u  : return false;
    case 0x0B94u  : return false;


    



    case 0x0F77u  : *a = 0x0FB2u; *b= 0x0F81u; return true;
    case 0x0F79u  : *a = 0x0FB3u; *b= 0x0F81u; return true;
    case 0x17BEu  : *a = 0x17C1u; *b= 0x17BEu; return true;
    case 0x17BFu  : *a = 0x17C1u; *b= 0x17BFu; return true;
    case 0x17C0u  : *a = 0x17C1u; *b= 0x17C0u; return true;
    case 0x17C4u  : *a = 0x17C1u; *b= 0x17C4u; return true;
    case 0x17C5u  : *a = 0x17C1u; *b= 0x17C5u; return true;
    case 0x1925u  : *a = 0x1920u; *b= 0x1923u; return true;
    case 0x1926u  : *a = 0x1920u; *b= 0x1924u; return true;
    case 0x1B3Cu  : *a = 0x1B42u; *b= 0x1B3Cu; return true;
    case 0x1112Eu  : *a = 0x11127u; *b= 0x11131u; return true;
    case 0x1112Fu  : *a = 0x11127u; *b= 0x11132u; return true;
#if 0
    
    
    case 0x0B57u  : *a = no decomp, -> RIGHT; return true;
    case 0x1C29u  : *a = no decomp, -> LEFT; return true;
    case 0xA9C0u  : *a = no decomp, -> RIGHT; return true;
    case 0x111BuF  : *a = no decomp, -> ABOVE; return true;
#endif
  }

  if ((ab == 0x0DDAu || hb_in_range (ab, 0x0DDCu, 0x0DDEu)))
  {
    

























    const indic_shape_plan_t *indic_plan = (const indic_shape_plan_t *) c->plan->data;

    hb_codepoint_t glyph;

    if (hb_options ().uniscribe_bug_compatible ||
	(c->font->get_glyph (ab, 0, &glyph) &&
	 indic_plan->pstf.would_substitute (&glyph, 1, c->font->face)))
    {
      
      *a = 0x0DD9u;
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

  
  if (a == 0x09AFu && b == 0x09BCu) { *ab = 0x09DFu; return true; }

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
  HB_OT_SHAPE_NORMALIZATION_MODE_COMPOSED_DIACRITICS_NO_SHORT_CIRCUIT,
  decompose_indic,
  compose_indic,
  setup_masks_indic,
  HB_OT_SHAPE_ZERO_WIDTH_MARKS_NONE,
  false, 
};
