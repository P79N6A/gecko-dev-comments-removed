

























#include "hb-ot-shape-complex-private.hh"
#include "hb-ot-shape-private.hh"




#define arabic_shaping_action() complex_var_u8_0() /* arabic shaping action */





enum {
  JOINING_TYPE_U		= 0,
  JOINING_TYPE_R		= 1,
  JOINING_TYPE_D		= 2,
  JOINING_TYPE_C		= JOINING_TYPE_D,
  JOINING_GROUP_ALAPH		= 3,
  JOINING_GROUP_DALATH_RISH	= 4,
  NUM_STATE_MACHINE_COLS	= 5,

  

  JOINING_TYPE_T = 6,
  JOINING_TYPE_X = 7  
};





#include "hb-ot-shape-complex-arabic-table.hh"

static unsigned int get_joining_type (hb_codepoint_t u, hb_unicode_general_category_t gen_cat)
{
  if (likely (hb_in_range<hb_codepoint_t> (u, JOINING_TABLE_FIRST, JOINING_TABLE_LAST))) {
    unsigned int j_type = joining_table[u - JOINING_TABLE_FIRST];
    if (likely (j_type != JOINING_TYPE_X))
      return j_type;
  }

  
  if (unlikely (hb_in_range<hb_codepoint_t> (u, 0x1800, 0x18AF)))
  {
    
    if (gen_cat == HB_UNICODE_GENERAL_CATEGORY_OTHER_LETTER || u == 0x1807 || u == 0x180A)
      return JOINING_TYPE_D;
  }

  if (unlikely (hb_in_range<hb_codepoint_t> (u, 0x200C, 0x200D))) {
    return u == 0x200C ? JOINING_TYPE_U : JOINING_TYPE_C;
  }

  return (FLAG(gen_cat) & (FLAG(HB_UNICODE_GENERAL_CATEGORY_NON_SPACING_MARK) | FLAG(HB_UNICODE_GENERAL_CATEGORY_ENCLOSING_MARK) | FLAG(HB_UNICODE_GENERAL_CATEGORY_FORMAT))) ?
	 JOINING_TYPE_T : JOINING_TYPE_U;
}

static hb_codepoint_t get_arabic_shape (hb_codepoint_t u, unsigned int shape)
{
  if (likely (hb_in_range<hb_codepoint_t> (u, SHAPING_TABLE_FIRST, SHAPING_TABLE_LAST)) && shape < 4)
    return shaping_table[u - SHAPING_TABLE_FIRST][shape];
  return u;
}

static uint16_t get_ligature (hb_codepoint_t first, hb_codepoint_t second)
{
  if (unlikely (!second)) return 0;
  for (unsigned i = 0; i < ARRAY_LENGTH (ligature_table); i++)
    if (ligature_table[i].first == first)
      for (unsigned j = 0; j < ARRAY_LENGTH (ligature_table[i].ligatures); j++)
	if (ligature_table[i].ligatures[j].second == second)
	  return ligature_table[i].ligatures[j].ligature;
  return 0;
}

static const hb_tag_t arabic_features[] =
{
  HB_TAG('i','n','i','t'),
  HB_TAG('m','e','d','i'),
  HB_TAG('f','i','n','a'),
  HB_TAG('i','s','o','l'),
  
  HB_TAG('m','e','d','2'),
  HB_TAG('f','i','n','2'),
  HB_TAG('f','i','n','3'),
  HB_TAG_NONE
};



enum {
  INIT,
  MEDI,
  FINA,
  ISOL,

  
  MED2,
  FIN2,
  FIN3,

  NONE,

  ARABIC_NUM_FEATURES = NONE
};

static const struct arabic_state_table_entry {
	uint8_t prev_action;
	uint8_t curr_action;
	uint16_t next_state;
} arabic_state_table[][NUM_STATE_MACHINE_COLS] =
{
  

  
  { {NONE,NONE,0}, {NONE,ISOL,1}, {NONE,ISOL,2}, {NONE,ISOL,1}, {NONE,ISOL,6}, },

  
  { {NONE,NONE,0}, {NONE,ISOL,1}, {NONE,ISOL,2}, {NONE,FIN2,5}, {NONE,ISOL,6}, },

  
  { {NONE,NONE,0}, {INIT,FINA,1}, {INIT,FINA,3}, {INIT,FINA,4}, {INIT,FINA,6}, },

  
  { {NONE,NONE,0}, {MEDI,FINA,1}, {MEDI,FINA,3}, {MEDI,FINA,4}, {MEDI,FINA,6}, },

  
  { {NONE,NONE,0}, {MED2,ISOL,1}, {MED2,ISOL,2}, {MED2,FIN2,5}, {MED2,ISOL,6}, },

  
  { {NONE,NONE,0}, {ISOL,ISOL,1}, {ISOL,ISOL,2}, {ISOL,FIN2,5}, {ISOL,ISOL,6}, },

  
  { {NONE,NONE,0}, {NONE,ISOL,1}, {NONE,ISOL,2}, {NONE,FIN3,5}, {NONE,ISOL,6}, }
};



static void
collect_features_arabic (hb_ot_shape_planner_t *plan)
{
  hb_ot_map_builder_t *map = &plan->map;

  









  map->add_bool_feature (HB_TAG('c','c','m','p'));
  map->add_bool_feature (HB_TAG('l','o','c','l'));

  map->add_gsub_pause (NULL);

  for (unsigned int i = 0; i < ARABIC_NUM_FEATURES; i++)
    map->add_bool_feature (arabic_features[i], false);

  map->add_gsub_pause (NULL);

  map->add_bool_feature (HB_TAG('r','l','i','g'));
  map->add_gsub_pause (NULL);

  map->add_bool_feature (HB_TAG('c','a','l','t'));
  map->add_gsub_pause (NULL);

  
  map->add_bool_feature (HB_TAG('c','s','w','h'));
}

struct arabic_shape_plan_t
{
  ASSERT_POD ();

  bool do_fallback;
  



  hb_mask_t mask_array[ARABIC_NUM_FEATURES + 1];
};

static void *
data_create_arabic (const hb_ot_shape_plan_t *plan)
{
  arabic_shape_plan_t *arabic_plan = (arabic_shape_plan_t *) calloc (1, sizeof (arabic_shape_plan_t));
  if (unlikely (!arabic_plan))
    return NULL;

  hb_mask_t total_masks = 0;
  for (unsigned int i = 0; i < ARABIC_NUM_FEATURES; i++) {
    arabic_plan->mask_array[i] = plan->map.get_1_mask (arabic_features[i]);
    total_masks |= arabic_plan->mask_array[i];
  }

  





  arabic_plan->do_fallback = 0 == total_masks;

  return arabic_plan;
}

static void
data_destroy_arabic (void *data)
{
  free (data);
}

static void
arabic_fallback_shape (hb_font_t *font, hb_buffer_t *buffer)
{
  
  if (buffer->props.script != HB_SCRIPT_ARABIC)
    return;

  unsigned int count = buffer->len;
  hb_codepoint_t glyph;

  
  for (unsigned int i = 0; i < count; i++) {
    hb_codepoint_t u = buffer->info[i].codepoint;
    hb_codepoint_t shaped = get_arabic_shape (u, buffer->info[i].arabic_shaping_action());
    if (shaped != u && font->get_glyph (shaped, 0, &glyph))
      buffer->info[i].codepoint = shaped;
  }

  
  buffer->clear_output ();
  for (buffer->idx = 0; buffer->idx + 1 < count;) {
    hb_codepoint_t ligature = get_ligature (buffer->cur().codepoint,
					    buffer->cur(+1).codepoint);
    if (likely (!ligature) || !(font->get_glyph (ligature, 0, &glyph))) {
      buffer->next_glyph ();
      continue;
    }

    buffer->replace_glyphs (2, 1, &ligature);

    

  }
  for (; buffer->idx < count;)
      buffer->next_glyph ();
  buffer->swap_buffers ();
}

static void
arabic_joining (hb_buffer_t *buffer)
{
  unsigned int count = buffer->len;
  unsigned int prev = 0, state = 0;

  HB_BUFFER_ALLOCATE_VAR (buffer, arabic_shaping_action);

  for (unsigned int i = 0; i < count; i++)
  {
    unsigned int this_type = get_joining_type (buffer->info[i].codepoint, _hb_glyph_info_get_general_category (&buffer->info[i]));

    if (unlikely (this_type == JOINING_TYPE_T)) {
      buffer->info[i].arabic_shaping_action() = NONE;
      continue;
    }

    const arabic_state_table_entry *entry = &arabic_state_table[state][this_type];

    if (entry->prev_action != NONE)
      buffer->info[prev].arabic_shaping_action() = entry->prev_action;

    buffer->info[i].arabic_shaping_action() = entry->curr_action;

    prev = i;
    state = entry->next_state;
  }

  HB_BUFFER_DEALLOCATE_VAR (buffer, arabic_shaping_action);
}

static void
preprocess_text_arabic (const hb_ot_shape_plan_t *plan,
			hb_buffer_t              *buffer,
			hb_font_t                *font)
{
  const arabic_shape_plan_t *arabic_plan = (const arabic_shape_plan_t *) plan->data;

  if (unlikely (arabic_plan->do_fallback))
  {
    arabic_joining (buffer);
    arabic_fallback_shape (font, buffer);
  }
}

static void
setup_masks_arabic (const hb_ot_shape_plan_t *plan,
		    hb_buffer_t              *buffer,
		    hb_font_t                *font)
{
  const arabic_shape_plan_t *arabic_plan = (const arabic_shape_plan_t *) plan->data;

  if (likely (!arabic_plan->do_fallback))
  {
    arabic_joining (buffer);
    unsigned int count = buffer->len;
    for (unsigned int i = 0; i < count; i++)
      buffer->info[i].mask |= arabic_plan->mask_array[buffer->info[i].arabic_shaping_action()];
  }
}

const hb_ot_complex_shaper_t _hb_ot_complex_shaper_arabic =
{
  "arabic",
  collect_features_arabic,
  NULL, 
  data_create_arabic,
  data_destroy_arabic,
  preprocess_text_arabic,
  NULL, 
  setup_masks_arabic,
  true, 
};
