

























#include "hb-ot-shape-complex-private.hh"

HB_BEGIN_DECLS



#define arabic_shaping_action() var2.u32 /* arabic shaping action */





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





#include "hb-ot-shape-complex-arabic-table.h"

static unsigned int get_joining_type (hb_codepoint_t u, hb_category_t gen_cat)
{
  

  if (likely (JOINING_TABLE_FIRST <= u && u <= JOINING_TABLE_LAST)) {
    unsigned int j_type = joining_table[u - JOINING_TABLE_FIRST];
    if (likely (j_type != JOINING_TYPE_X))
      return j_type;
  }

  if (unlikely ((u & ~(0x200C^0x200D)) == 0x200C)) {
    return u == 0x200C ? JOINING_TYPE_U : JOINING_TYPE_C;
  }

  return ((1<<gen_cat) & ((1<<HB_CATEGORY_NON_SPACING_MARK)|(1<<HB_CATEGORY_ENCLOSING_MARK)|(1<<HB_CATEGORY_FORMAT))) ?
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
	uint8_t next_state;
	uint8_t padding;
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
_hb_ot_shape_complex_collect_features_arabic	(hb_ot_shape_plan_t *plan, const hb_segment_properties_t  *props)
{
  unsigned int num_features = props->script == HB_SCRIPT_SYRIAC ? SYRIAC_NUM_FEATURES : COMMON_NUM_FEATURES;
  for (unsigned int i = 0; i < num_features; i++)
    plan->map.add_bool_feature (arabic_syriac_features[i], EARLY_PRIORITY, false);
}

void
_hb_ot_shape_complex_setup_masks_arabic	(hb_ot_shape_context_t *c)
{
  unsigned int count = c->buffer->len;
  unsigned int prev = 0, state = 0;

  for (unsigned int i = 0; i < count; i++)
  {
    unsigned int this_type = get_joining_type (c->buffer->info[i].codepoint, (hb_category_t) c->buffer->info[i].general_category());

    if (unlikely (this_type == JOINING_TYPE_T)) {
      c->buffer->info[i].arabic_shaping_action() = NONE;
      continue;
    }

    const arabic_state_table_entry *entry = &arabic_state_table[state][this_type];

    if (entry->prev_action != NONE)
      c->buffer->info[prev].arabic_shaping_action() = entry->prev_action;

    c->buffer->info[i].arabic_shaping_action() = entry->curr_action;

    prev = i;
    state = entry->next_state;
  }

  hb_mask_t mask_array[TOTAL_NUM_FEATURES + 1] = {0};
  unsigned int num_masks = c->buffer->props.script == HB_SCRIPT_SYRIAC ? SYRIAC_NUM_FEATURES : COMMON_NUM_FEATURES;
  for (unsigned int i = 0; i < num_masks; i++)
    mask_array[i] = c->plan->map.get_1_mask (arabic_syriac_features[i]);

  for (unsigned int i = 0; i < count; i++)
    c->buffer->info[i].mask |= mask_array[c->buffer->info[i].arabic_shaping_action()];
}


HB_END_DECLS
