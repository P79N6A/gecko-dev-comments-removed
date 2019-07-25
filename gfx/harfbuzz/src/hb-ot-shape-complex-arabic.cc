

























#include "hb-ot-shape-complex-private.hh"




#define arabic_shaping_action() complex_var_temporary_u16() /* arabic shaping action */





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



static const hb_tag_t arabic_syriac_features[] =
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

  COMMON_NUM_FEATURES = 4,
  SYRIAC_NUM_FEATURES = 7,
  TOTAL_NUM_FEATURES = NONE
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



void
_hb_ot_shape_complex_collect_features_arabic (hb_ot_map_builder_t *map, const hb_segment_properties_t  *props)
{
  









  map->add_bool_feature (HB_TAG('c','c','m','p'));
  map->add_bool_feature (HB_TAG('l','o','c','l'));

  map->add_gsub_pause (NULL, NULL);

  unsigned int num_features = props->script == HB_SCRIPT_SYRIAC ? SYRIAC_NUM_FEATURES : COMMON_NUM_FEATURES;
  for (unsigned int i = 0; i < num_features; i++)
    map->add_bool_feature (arabic_syriac_features[i], false);

  map->add_gsub_pause (NULL, NULL);

  map->add_bool_feature (HB_TAG('r','l','i','g'));
  map->add_gsub_pause (NULL, NULL);

  map->add_bool_feature (HB_TAG('c','a','l','t'));
  map->add_gsub_pause (NULL, NULL);

  
  map->add_bool_feature (HB_TAG('c','s','w','h'));
}

bool
_hb_ot_shape_complex_prefer_decomposed_arabic (void)
{
  return FALSE;
}

void
_hb_ot_shape_complex_setup_masks_arabic (hb_ot_map_t *map, hb_buffer_t *buffer)
{
  unsigned int count = buffer->len;
  unsigned int prev = 0, state = 0;

  HB_BUFFER_ALLOCATE_VAR (buffer, arabic_shaping_action);

  for (unsigned int i = 0; i < count; i++)
  {
    unsigned int this_type = get_joining_type (buffer->info[i].codepoint, (hb_unicode_general_category_t) buffer->info[i].general_category());

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

  hb_mask_t mask_array[TOTAL_NUM_FEATURES + 1] = {0};
  unsigned int num_masks = buffer->props.script == HB_SCRIPT_SYRIAC ? SYRIAC_NUM_FEATURES : COMMON_NUM_FEATURES;
  for (unsigned int i = 0; i < num_masks; i++)
    mask_array[i] = map->get_1_mask (arabic_syriac_features[i]);

  for (unsigned int i = 0; i < count; i++)
    buffer->info[i].mask |= mask_array[buffer->info[i].arabic_shaping_action()];

  HB_BUFFER_DEALLOCATE_VAR (buffer, arabic_shaping_action);
}


