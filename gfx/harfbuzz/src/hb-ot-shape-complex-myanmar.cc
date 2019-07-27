

























#include "hb-ot-shape-complex-indic-private.hh"


#define myanmar_category() complex_var_u8_0() /* myanmar_category_t */
#define myanmar_position() complex_var_u8_1() /* myanmar_position_t */






static const hb_tag_t
basic_features[] =
{
  



  HB_TAG('r','p','h','f'),
  HB_TAG('p','r','e','f'),
  HB_TAG('b','l','w','f'),
  HB_TAG('p','s','t','f'),
};
static const hb_tag_t
other_features[] =
{
  



  HB_TAG('p','r','e','s'),
  HB_TAG('a','b','v','s'),
  HB_TAG('b','l','w','s'),
  HB_TAG('p','s','t','s'),
  
  HB_TAG('d','i','s','t'),
  







  HB_TAG('a','b','v','m'),
  HB_TAG('b','l','w','m'),
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
collect_features_myanmar (hb_ot_shape_planner_t *plan)
{
  hb_ot_map_builder_t *map = &plan->map;

  
  map->add_gsub_pause (setup_syllables);

  map->add_global_bool_feature (HB_TAG('l','o','c','l'));
  

  map->add_global_bool_feature (HB_TAG('c','c','m','p'));


  map->add_gsub_pause (initial_reordering);
  for (unsigned int i = 0; i < ARRAY_LENGTH (basic_features); i++)
  {
    map->add_feature (basic_features[i], 1, F_GLOBAL | F_MANUAL_ZWJ);
    map->add_gsub_pause (NULL);
  }
  map->add_gsub_pause (final_reordering);
  for (unsigned int i = 0; i < ARRAY_LENGTH (other_features); i++)
    map->add_feature (other_features[i], 1, F_GLOBAL | F_MANUAL_ZWJ);
}

static void
override_features_myanmar (hb_ot_shape_planner_t *plan)
{
  plan->map.add_feature (HB_TAG('l','i','g','a'), 0, F_GLOBAL);
}


enum syllable_type_t {
  consonant_syllable,
  punctuation_cluster,
  broken_cluster,
  non_myanmar_cluster,
};

#include "hb-ot-shape-complex-myanmar-machine.hh"




enum myanmar_category_t {
  OT_As  = 18, 
  OT_D   = 19, 
  OT_D0  = 20, 
  OT_DB  = OT_N, 
  OT_GB  = OT_PLACEHOLDER,
  OT_MH  = 21, 
  OT_MR  = 22, 
  OT_MW  = 23, 
  OT_MY  = 24, 
  OT_PT  = 25, 
  OT_VAbv = 26,
  OT_VBlw = 27,
  OT_VPre = 28,
  OT_VPst = 29,
  OT_VS   = 30, 
  OT_P    = 31  
};


static inline bool
is_one_of (const hb_glyph_info_t &info, unsigned int flags)
{
  
  if (_hb_glyph_info_ligated (&info)) return false;
  return !!(FLAG (info.myanmar_category()) & flags);
}

static inline bool
is_consonant (const hb_glyph_info_t &info)
{
  return is_one_of (info, CONSONANT_FLAGS);
}


static inline void
set_myanmar_properties (hb_glyph_info_t &info)
{
  hb_codepoint_t u = info.codepoint;
  unsigned int type = hb_indic_get_categories (u);
  indic_category_t cat = (indic_category_t) (type & 0x7Fu);
  indic_position_t pos = (indic_position_t) (type >> 8);

  


  if (unlikely (hb_in_range (u, 0xFE00u, 0xFE0Fu)))
    cat = (indic_category_t) OT_VS;

  switch (u)
  {
    case 0x104Eu:
      cat = (indic_category_t) OT_C; 
      break;

    case 0x002Du: case 0x00A0u: case 0x00D7u: case 0x2012u:
    case 0x2013u: case 0x2014u: case 0x2015u: case 0x2022u:
    case 0x25CCu: case 0x25FBu: case 0x25FCu: case 0x25FDu:
    case 0x25FEu:
      cat = (indic_category_t) OT_GB;
      break;

    case 0x1004u: case 0x101Bu: case 0x105Au:
      cat = (indic_category_t) OT_Ra;
      break;

    case 0x1032u: case 0x1036u:
      cat = (indic_category_t) OT_A;
      break;

    case 0x103Au:
      cat = (indic_category_t) OT_As;
      break;

    case 0x1041u: case 0x1042u: case 0x1043u: case 0x1044u:
    case 0x1045u: case 0x1046u: case 0x1047u: case 0x1048u:
    case 0x1049u: case 0x1090u: case 0x1091u: case 0x1092u:
    case 0x1093u: case 0x1094u: case 0x1095u: case 0x1096u:
    case 0x1097u: case 0x1098u: case 0x1099u:
      cat = (indic_category_t) OT_D;
      break;

    case 0x1040u:
      cat = (indic_category_t) OT_D; 
      break;

    case 0x103Eu: case 0x1060u:
      cat = (indic_category_t) OT_MH;
      break;

    case 0x103Cu:
      cat = (indic_category_t) OT_MR;
      break;

    case 0x103Du: case 0x1082u:
      cat = (indic_category_t) OT_MW;
      break;

    case 0x103Bu: case 0x105Eu: case 0x105Fu:
      cat = (indic_category_t) OT_MY;
      break;

    case 0x1063u: case 0x1064u: case 0x1069u: case 0x106Au:
    case 0x106Bu: case 0x106Cu: case 0x106Du: case 0xAA7Bu:
      cat = (indic_category_t) OT_PT;
      break;

    case 0x1038u: case 0x1087u: case 0x1088u: case 0x1089u:
    case 0x108Au: case 0x108Bu: case 0x108Cu: case 0x108Du:
    case 0x108Fu: case 0x109Au: case 0x109Bu: case 0x109Cu:
      cat = (indic_category_t) OT_SM;
      break;

    case 0x104Au: case 0x104Bu:
      cat = (indic_category_t) OT_P;
      break;
  }

  if (cat == OT_M)
  {
    switch ((int) pos)
    {
      case POS_PRE_C:	cat = (indic_category_t) OT_VPre;
			pos = POS_PRE_M;                  break;
      case POS_ABOVE_C:	cat = (indic_category_t) OT_VAbv; break;
      case POS_BELOW_C:	cat = (indic_category_t) OT_VBlw; break;
      case POS_POST_C:	cat = (indic_category_t) OT_VPst; break;
    }
  }

  info.myanmar_category() = (myanmar_category_t) cat;
  info.myanmar_position() = pos;
}



static void
setup_masks_myanmar (const hb_ot_shape_plan_t *plan HB_UNUSED,
		   hb_buffer_t              *buffer,
		   hb_font_t                *font HB_UNUSED)
{
  HB_BUFFER_ALLOCATE_VAR (buffer, myanmar_category);
  HB_BUFFER_ALLOCATE_VAR (buffer, myanmar_position);

  


  unsigned int count = buffer->len;
  hb_glyph_info_t *info = buffer->info;
  for (unsigned int i = 0; i < count; i++)
    set_myanmar_properties (info[i]);
}

static void
setup_syllables (const hb_ot_shape_plan_t *plan HB_UNUSED,
		 hb_font_t *font HB_UNUSED,
		 hb_buffer_t *buffer)
{
  find_syllables (buffer);
}

static int
compare_myanmar_order (const hb_glyph_info_t *pa, const hb_glyph_info_t *pb)
{
  int a = pa->myanmar_position();
  int b = pb->myanmar_position();

  return a < b ? -1 : a == b ? 0 : +1;
}





static void
initial_reordering_consonant_syllable (const hb_ot_shape_plan_t *plan,
				       hb_face_t *face,
				       hb_buffer_t *buffer,
				       unsigned int start, unsigned int end)
{
  hb_glyph_info_t *info = buffer->info;

  unsigned int base = end;
  bool has_reph = false;

  {
    unsigned int limit = start;
    if (start + 3 <= end &&
	info[start  ].myanmar_category() == OT_Ra &&
	info[start+1].myanmar_category() == OT_As &&
	info[start+2].myanmar_category() == OT_H)
    {
      limit += 3;
      base = start;
      has_reph = true;
    }

    {
      if (!has_reph)
	base = limit;

      for (unsigned int i = limit; i < end; i++)
	if (is_consonant (info[i]))
	{
	  base = i;
	  break;
	}
    }
  }

  
  {
    unsigned int i = start;
    for (; i < start + (has_reph ? 3 : 0); i++)
      info[i].myanmar_position() = POS_AFTER_MAIN;
    for (; i < base; i++)
      info[i].myanmar_position() = POS_PRE_C;
    if (i < end)
    {
      info[i].myanmar_position() = POS_BASE_C;
      i++;
    }
    indic_position_t pos = POS_AFTER_MAIN;
    

    for (; i < end; i++)
    {
      if (info[i].myanmar_category() == OT_MR) 
      {
	info[i].myanmar_position() = POS_PRE_C;
	continue;
      }
      if (info[i].myanmar_position() < POS_BASE_C) 
      {
	continue;
      }

      if (pos == POS_AFTER_MAIN && info[i].myanmar_category() == OT_VBlw)
      {
	pos = POS_BELOW_C;
	info[i].myanmar_position() = pos;
	continue;
      }

      if (pos == POS_BELOW_C && info[i].myanmar_category() == OT_A)
      {
	info[i].myanmar_position() = POS_BEFORE_SUB;
	continue;
      }
      if (pos == POS_BELOW_C && info[i].myanmar_category() == OT_VBlw)
      {
	info[i].myanmar_position() = pos;
	continue;
      }
      if (pos == POS_BELOW_C && info[i].myanmar_category() != OT_A)
      {
        pos = POS_AFTER_SUB;
	info[i].myanmar_position() = pos;
	continue;
      }
      info[i].myanmar_position() = pos;
    }
  }

  buffer->merge_clusters (start, end);
  
  hb_bubble_sort (info + start, end - start, compare_myanmar_order);
}

static void
initial_reordering_broken_cluster (const hb_ot_shape_plan_t *plan,
				   hb_face_t *face,
				   hb_buffer_t *buffer,
				   unsigned int start, unsigned int end)
{
  
  initial_reordering_consonant_syllable (plan, face, buffer, start, end);
}

static void
initial_reordering_punctuation_cluster (const hb_ot_shape_plan_t *plan HB_UNUSED,
					hb_face_t *face HB_UNUSED,
					hb_buffer_t *buffer HB_UNUSED,
					unsigned int start HB_UNUSED, unsigned int end HB_UNUSED)
{
  

}

static void
initial_reordering_non_myanmar_cluster (const hb_ot_shape_plan_t *plan HB_UNUSED,
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
  case consonant_syllable:	initial_reordering_consonant_syllable  (plan, face, buffer, start, end); return;
  case punctuation_cluster:	initial_reordering_punctuation_cluster (plan, face, buffer, start, end); return;
  case broken_cluster:		initial_reordering_broken_cluster      (plan, face, buffer, start, end); return;
  case non_myanmar_cluster:	initial_reordering_non_myanmar_cluster (plan, face, buffer, start, end); return;
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
  set_myanmar_properties (dottedcircle);
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
final_reordering (const hb_ot_shape_plan_t *plan,
		  hb_font_t *font HB_UNUSED,
		  hb_buffer_t *buffer)
{
  hb_glyph_info_t *info = buffer->info;
  unsigned int count = buffer->len;

  
  for (unsigned int i = 0; i < count; i++)
    info[i].syllable() = 0;

  HB_BUFFER_DEALLOCATE_VAR (buffer, myanmar_category);
  HB_BUFFER_DEALLOCATE_VAR (buffer, myanmar_position);
}




const hb_ot_complex_shaper_t _hb_ot_complex_shaper_myanmar_old =
{
  "default",
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  HB_OT_SHAPE_NORMALIZATION_MODE_DEFAULT,
  NULL, 
  NULL, 
  NULL, 
  HB_OT_SHAPE_ZERO_WIDTH_MARKS_BY_GDEF_LATE,
  true, 
};

const hb_ot_complex_shaper_t _hb_ot_complex_shaper_myanmar =
{
  "myanmar",
  collect_features_myanmar,
  override_features_myanmar,
  NULL, 
  NULL, 
  NULL, 
  HB_OT_SHAPE_NORMALIZATION_MODE_COMPOSED_DIACRITICS_NO_SHORT_CIRCUIT,
  NULL, 
  NULL, 
  setup_masks_myanmar,
  HB_OT_SHAPE_ZERO_WIDTH_MARKS_BY_GDEF_EARLY,
  false, 
};
