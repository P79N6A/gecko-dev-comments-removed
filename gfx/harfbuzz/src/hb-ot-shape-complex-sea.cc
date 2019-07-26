

























#include "hb-ot-shape-complex-indic-private.hh"


#define sea_category() complex_var_u8_0() /* indic_category_t */
#define sea_position() complex_var_u8_1() /* indic_position_t */








static const hb_tag_t
sea_features[] =
{
  



  HB_TAG('p','r','e','f'),
  HB_TAG('a','b','v','f'),
  HB_TAG('b','l','w','f'),
  HB_TAG('p','s','t','f'),
  



  HB_TAG('p','r','e','s'),
  HB_TAG('a','b','v','s'),
  HB_TAG('b','l','w','s'),
  HB_TAG('p','s','t','s'),
  
  HB_TAG('d','i','s','t'),
};




enum {
  _PREF,
  _ABVF,
  _BLWF,
  _PSTF,

  _PRES,
  _ABVS,
  _BLWS,
  _PSTS,
  _DIST,

  SEA_NUM_FEATURES,
  SEA_BASIC_FEATURES = _PRES 
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
collect_features_sea (hb_ot_shape_planner_t *plan)
{
  hb_ot_map_builder_t *map = &plan->map;

  
  map->add_gsub_pause (setup_syllables);

  map->add_bool_feature (HB_TAG('l','o','c','l'));
  

  map->add_bool_feature (HB_TAG('c','c','m','p'));


  unsigned int i = 0;
  map->add_gsub_pause (initial_reordering);
  for (; i < SEA_BASIC_FEATURES; i++) {
    map->add_bool_feature (sea_features[i]);
    map->add_gsub_pause (NULL);
  }
  map->add_gsub_pause (final_reordering);
  for (; i < SEA_NUM_FEATURES; i++) {
    map->add_bool_feature (sea_features[i]);
  }
}

static void
override_features_sea (hb_ot_shape_planner_t *plan)
{
  plan->map.add_feature (HB_TAG('l','i','g','a'), 0, true);
}


enum syllable_type_t {
  consonant_syllable,
  broken_cluster,
  non_sea_cluster,
};

#include "hb-ot-shape-complex-sea-machine.hh"




enum sea_category_t {

  OT_GB   = 12, 

  OT_IV   = 2,  
  OT_MR   = 22, 

  OT_VAbv = 26,
  OT_VBlw = 27,
  OT_VPre = 28,
  OT_VPst = 29,
  OT_T    = 3,  

};

static inline void
set_sea_properties (hb_glyph_info_t &info)
{
  hb_codepoint_t u = info.codepoint;
  unsigned int type = hb_indic_get_categories (u);
  indic_category_t cat = (indic_category_t) (type & 0x7F);
  indic_position_t pos = (indic_position_t) (type >> 8);

  
  if (u == 0x1A55 || u == 0xAA34)
    cat = (indic_category_t) OT_MR;

  if (cat == OT_M)
  {
    switch ((int) pos)
    {
      case POS_PRE_C:	cat = (indic_category_t) OT_VPre; break;
      case POS_ABOVE_C:	cat = (indic_category_t) OT_VAbv; break;
      case POS_BELOW_C:	cat = (indic_category_t) OT_VBlw; break;
      case POS_POST_C:	cat = (indic_category_t) OT_VPst; break;
    }
  }

  info.sea_category() = (sea_category_t) cat;
  info.sea_position() = pos;
}


static void
setup_masks_sea (const hb_ot_shape_plan_t *plan HB_UNUSED,
		 hb_buffer_t              *buffer,
		 hb_font_t                *font HB_UNUSED)
{
  HB_BUFFER_ALLOCATE_VAR (buffer, sea_category);
  HB_BUFFER_ALLOCATE_VAR (buffer, sea_position);

  


  unsigned int count = buffer->len;
  for (unsigned int i = 0; i < count; i++)
    set_sea_properties (buffer->info[i]);
}

static void
setup_syllables (const hb_ot_shape_plan_t *plan HB_UNUSED,
		 hb_font_t *font HB_UNUSED,
		 hb_buffer_t *buffer)
{
  find_syllables (buffer);
}

static int
compare_sea_order (const hb_glyph_info_t *pa, const hb_glyph_info_t *pb)
{
  int a = pa->sea_position();
  int b = pb->sea_position();

  return a < b ? -1 : a == b ? 0 : +1;
}


static void
initial_reordering_consonant_syllable (const hb_ot_shape_plan_t *plan,
				       hb_face_t *face,
				       hb_buffer_t *buffer,
				       unsigned int start, unsigned int end)
{
  hb_glyph_info_t *info = buffer->info;
  unsigned int base = start;

  
  unsigned int i = start;
  for (; i < base; i++)
    info[i].sea_position() = POS_PRE_C;
  if (i < end)
  {
    info[i].sea_position() = POS_BASE_C;
    i++;
  }
  for (; i < end; i++)
  {
    if (info[i].sea_category() == OT_MR) 
    {
      info[i].sea_position() = POS_PRE_C;
      continue;
    }
    if (info[i].sea_category() == OT_VPre) 
    {
      info[i].sea_position() = POS_PRE_M;
      continue;
    }

    info[i].sea_position() = POS_AFTER_MAIN;
  }

  buffer->merge_clusters (start, end);
  
  hb_bubble_sort (info + start, end - start, compare_sea_order);
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
initial_reordering_non_sea_cluster (const hb_ot_shape_plan_t *plan HB_UNUSED,
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
  case broken_cluster:		initial_reordering_broken_cluster      (plan, face, buffer, start, end); return;
  case non_sea_cluster:	initial_reordering_non_sea_cluster (plan, face, buffer, start, end); return;
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
  set_sea_properties (dottedcircle);
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

  HB_BUFFER_DEALLOCATE_VAR (buffer, sea_category);
  HB_BUFFER_DEALLOCATE_VAR (buffer, sea_position);
}


static hb_ot_shape_normalization_mode_t
normalization_preference_sea (const hb_segment_properties_t *props HB_UNUSED)
{
  return HB_OT_SHAPE_NORMALIZATION_MODE_COMPOSED_DIACRITICS_NO_SHORT_CIRCUIT;
}


const hb_ot_complex_shaper_t _hb_ot_complex_shaper_sea =
{
  "sea",
  collect_features_sea,
  override_features_sea,
  NULL, 
  NULL, 
  NULL, 
  normalization_preference_sea,
  NULL, 
  NULL, 
  setup_masks_sea,
  HB_OT_SHAPE_ZERO_WIDTH_MARKS_NONE,
  false, 
};
