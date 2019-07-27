

























#include "hb-ot-shape-complex-private.hh"
#include "hb-ot-shape-private.hh"



#define arabic_shaping_action() complex_var_u8_0() /* arabic shaping action */





enum {
  JOINING_TYPE_U		= 0,
  JOINING_TYPE_L		= 1,
  JOINING_TYPE_R		= 2,
  JOINING_TYPE_D		= 3,
  JOINING_TYPE_C		= JOINING_TYPE_D,
  JOINING_GROUP_ALAPH		= 4,
  JOINING_GROUP_DALATH_RISH	= 5,
  NUM_STATE_MACHINE_COLS	= 6,

  JOINING_TYPE_T = 7,
  JOINING_TYPE_X = 8  
};





#include "hb-ot-shape-complex-arabic-table.hh"

static unsigned int get_joining_type (hb_codepoint_t u, hb_unicode_general_category_t gen_cat)
{
  unsigned int j_type = joining_type(u);
  if (likely (j_type != JOINING_TYPE_X))
    return j_type;

  return (FLAG(gen_cat) &
	  (FLAG(HB_UNICODE_GENERAL_CATEGORY_NON_SPACING_MARK) |
	   FLAG(HB_UNICODE_GENERAL_CATEGORY_ENCLOSING_MARK) |
	   FLAG(HB_UNICODE_GENERAL_CATEGORY_FORMAT))
	 ) ?  JOINING_TYPE_T : JOINING_TYPE_U;
}

#define FEATURE_IS_SYRIAC(tag) hb_in_range<unsigned char> ((unsigned char) (tag), '2', '3')

static const hb_tag_t arabic_features[] =
{
  HB_TAG('i','s','o','l'),
  HB_TAG('f','i','n','a'),
  HB_TAG('f','i','n','2'),
  HB_TAG('f','i','n','3'),
  HB_TAG('m','e','d','i'),
  HB_TAG('m','e','d','2'),
  HB_TAG('i','n','i','t'),
  HB_TAG_NONE
};



enum {
  ISOL,
  FINA,
  FIN2,
  FIN3,
  MEDI,
  MED2,
  INIT,

  NONE,

  ARABIC_NUM_FEATURES = NONE
};

static const struct arabic_state_table_entry {
	uint8_t prev_action;
	uint8_t curr_action;
	uint16_t next_state;
} arabic_state_table[][NUM_STATE_MACHINE_COLS] =
{
  

  
  { {NONE,NONE,0}, {NONE,ISOL,2}, {NONE,ISOL,1}, {NONE,ISOL,2}, {NONE,ISOL,1}, {NONE,ISOL,6}, },

  
  { {NONE,NONE,0}, {NONE,ISOL,2}, {NONE,ISOL,1}, {NONE,ISOL,2}, {NONE,FIN2,5}, {NONE,ISOL,6}, },

  
  { {NONE,NONE,0}, {NONE,ISOL,2}, {INIT,FINA,1}, {INIT,FINA,3}, {INIT,FINA,4}, {INIT,FINA,6}, },

  
  { {NONE,NONE,0}, {NONE,ISOL,2}, {MEDI,FINA,1}, {MEDI,FINA,3}, {MEDI,FINA,4}, {MEDI,FINA,6}, },

  
  { {NONE,NONE,0}, {NONE,ISOL,2}, {MED2,ISOL,1}, {MED2,ISOL,2}, {MED2,FIN2,5}, {MED2,ISOL,6}, },

  
  { {NONE,NONE,0}, {NONE,ISOL,2}, {ISOL,ISOL,1}, {ISOL,ISOL,2}, {ISOL,FIN2,5}, {ISOL,ISOL,6}, },

  
  { {NONE,NONE,0}, {NONE,ISOL,2}, {NONE,ISOL,1}, {NONE,ISOL,2}, {NONE,FIN3,5}, {NONE,ISOL,6}, }
};


static void
nuke_joiners (const hb_ot_shape_plan_t *plan,
	      hb_font_t *font,
	      hb_buffer_t *buffer);

static void
arabic_fallback_shape (const hb_ot_shape_plan_t *plan,
		       hb_font_t *font,
		       hb_buffer_t *buffer);

static void
collect_features_arabic (hb_ot_shape_planner_t *plan)
{
  hb_ot_map_builder_t *map = &plan->map;

  


















  map->add_gsub_pause (nuke_joiners);

  map->add_global_bool_feature (HB_TAG('c','c','m','p'));
  map->add_global_bool_feature (HB_TAG('l','o','c','l'));

  map->add_gsub_pause (NULL);

  for (unsigned int i = 0; i < ARABIC_NUM_FEATURES; i++)
  {
    bool has_fallback = plan->props.script == HB_SCRIPT_ARABIC && !FEATURE_IS_SYRIAC (arabic_features[i]);
    map->add_feature (arabic_features[i], 1, has_fallback ? F_HAS_FALLBACK : F_NONE);
    map->add_gsub_pause (NULL);
  }

  map->add_feature (HB_TAG('r','l','i','g'), 1, F_GLOBAL|F_HAS_FALLBACK);
  if (plan->props.script == HB_SCRIPT_ARABIC)
    map->add_gsub_pause (arabic_fallback_shape);

  map->add_global_bool_feature (HB_TAG('c','a','l','t'));
  map->add_gsub_pause (NULL);

  







  
  map->add_global_bool_feature (HB_TAG('m','s','e','t'));
}

#include "hb-ot-shape-complex-arabic-fallback.hh"

struct arabic_shape_plan_t
{
  ASSERT_POD ();

  



  hb_mask_t mask_array[ARABIC_NUM_FEATURES + 1];

  bool do_fallback;
  arabic_fallback_plan_t *fallback_plan;
};

static void *
data_create_arabic (const hb_ot_shape_plan_t *plan)
{
  arabic_shape_plan_t *arabic_plan = (arabic_shape_plan_t *) calloc (1, sizeof (arabic_shape_plan_t));
  if (unlikely (!arabic_plan))
    return NULL;

  arabic_plan->do_fallback = plan->props.script == HB_SCRIPT_ARABIC;
  for (unsigned int i = 0; i < ARABIC_NUM_FEATURES; i++) {
    arabic_plan->mask_array[i] = plan->map.get_1_mask (arabic_features[i]);
    arabic_plan->do_fallback = arabic_plan->do_fallback &&
			       (FEATURE_IS_SYRIAC (arabic_features[i]) ||
			        plan->map.needs_fallback (arabic_features[i]));
  }

  return arabic_plan;
}

static void
data_destroy_arabic (void *data)
{
  arabic_shape_plan_t *arabic_plan = (arabic_shape_plan_t *) data;

  arabic_fallback_plan_destroy (arabic_plan->fallback_plan);

  free (data);
}

static void
arabic_joining (hb_buffer_t *buffer)
{
  unsigned int count = buffer->len;
  hb_glyph_info_t *info = buffer->info;
  unsigned int prev = (unsigned int) -1, state = 0;

  
  for (unsigned int i = 0; i < buffer->context_len[0]; i++)
  {
    unsigned int this_type = get_joining_type (buffer->context[0][i], buffer->unicode->general_category (buffer->context[0][i]));

    if (unlikely (this_type == JOINING_TYPE_T))
      continue;

    const arabic_state_table_entry *entry = &arabic_state_table[state][this_type];
    state = entry->next_state;
    break;
  }

  for (unsigned int i = 0; i < count; i++)
  {
    unsigned int this_type = get_joining_type (info[i].codepoint, _hb_glyph_info_get_general_category (&info[i]));

    if (unlikely (this_type == JOINING_TYPE_T)) {
      info[i].arabic_shaping_action() = NONE;
      continue;
    }

    const arabic_state_table_entry *entry = &arabic_state_table[state][this_type];

    if (entry->prev_action != NONE && prev != (unsigned int) -1)
      info[prev].arabic_shaping_action() = entry->prev_action;

    info[i].arabic_shaping_action() = entry->curr_action;

    prev = i;
    state = entry->next_state;
  }

  for (unsigned int i = 0; i < buffer->context_len[1]; i++)
  {
    unsigned int this_type = get_joining_type (buffer->context[1][i], buffer->unicode->general_category (buffer->context[1][i]));

    if (unlikely (this_type == JOINING_TYPE_T))
      continue;

    const arabic_state_table_entry *entry = &arabic_state_table[state][this_type];
    if (entry->prev_action != NONE && prev != (unsigned int) -1)
      info[prev].arabic_shaping_action() = entry->prev_action;
    break;
  }
}

static void
mongolian_variation_selectors (hb_buffer_t *buffer)
{
  
  unsigned int count = buffer->len;
  hb_glyph_info_t *info = buffer->info;
  for (unsigned int i = 1; i < count; i++)
    if (unlikely (hb_in_range (info[i].codepoint, 0x180Bu, 0x180Du)))
      info[i].arabic_shaping_action() = info[i - 1].arabic_shaping_action();
}

static void
setup_masks_arabic (const hb_ot_shape_plan_t *plan,
		    hb_buffer_t              *buffer,
		    hb_font_t                *font HB_UNUSED)
{
  HB_BUFFER_ALLOCATE_VAR (buffer, arabic_shaping_action);

  const arabic_shape_plan_t *arabic_plan = (const arabic_shape_plan_t *) plan->data;

  arabic_joining (buffer);
  if (plan->props.script == HB_SCRIPT_MONGOLIAN)
    mongolian_variation_selectors (buffer);

  unsigned int count = buffer->len;
  hb_glyph_info_t *info = buffer->info;
  for (unsigned int i = 0; i < count; i++)
    info[i].mask |= arabic_plan->mask_array[info[i].arabic_shaping_action()];

  HB_BUFFER_DEALLOCATE_VAR (buffer, arabic_shaping_action);
}


static void
nuke_joiners (const hb_ot_shape_plan_t *plan HB_UNUSED,
	      hb_font_t *font HB_UNUSED,
	      hb_buffer_t *buffer)
{
  unsigned int count = buffer->len;
  hb_glyph_info_t *info = buffer->info;
  for (unsigned int i = 0; i < count; i++)
    if (_hb_glyph_info_is_zwj (&info[i]))
      _hb_glyph_info_flip_joiners (&info[i]);
}

static void
arabic_fallback_shape (const hb_ot_shape_plan_t *plan,
		       hb_font_t *font,
		       hb_buffer_t *buffer)
{
  const arabic_shape_plan_t *arabic_plan = (const arabic_shape_plan_t *) plan->data;

  if (!arabic_plan->do_fallback)
    return;

retry:
  arabic_fallback_plan_t *fallback_plan = (arabic_fallback_plan_t *) hb_atomic_ptr_get (&arabic_plan->fallback_plan);
  if (unlikely (!fallback_plan))
  {
    
    fallback_plan = arabic_fallback_plan_create (plan, font);
    if (unlikely (!hb_atomic_ptr_cmpexch (&(const_cast<arabic_shape_plan_t *> (arabic_plan))->fallback_plan, NULL, fallback_plan))) {
      arabic_fallback_plan_destroy (fallback_plan);
      goto retry;
    }
  }

  arabic_fallback_plan_shape (fallback_plan, font, buffer);
}


const hb_ot_complex_shaper_t _hb_ot_complex_shaper_arabic =
{
  "arabic",
  collect_features_arabic,
  NULL, 
  data_create_arabic,
  data_destroy_arabic,
  NULL, 
  HB_OT_SHAPE_NORMALIZATION_MODE_DEFAULT,
  NULL, 
  NULL, 
  setup_masks_arabic,
  HB_OT_SHAPE_ZERO_WIDTH_MARKS_BY_GDEF_LATE,
  true, 
};
