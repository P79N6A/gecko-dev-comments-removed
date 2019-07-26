

























#include "hb-ot-shape-complex-private.hh"







static const hb_tag_t hangul_features[] =
{
  HB_TAG('l','j','m','o'),
  HB_TAG('v','j','m','o'),
  HB_TAG('t','j','m','o'),
  HB_TAG_NONE
};

static const hb_tag_t tibetan_features[] =
{
  HB_TAG('a','b','v','s'),
  HB_TAG('b','l','w','s'),
  HB_TAG('a','b','v','m'),
  HB_TAG('b','l','w','m'),
  HB_TAG_NONE
};

static void
collect_features_default (hb_ot_shape_planner_t *plan)
{
  const hb_tag_t *script_features = NULL;

  switch ((hb_tag_t) plan->props.script)
  {
    
    case HB_SCRIPT_HANGUL:
      script_features = hangul_features;
      break;

    
    case HB_SCRIPT_TIBETAN:
      script_features = tibetan_features;
      break;
  }

  for (; script_features && *script_features; script_features++)
    plan->map.add_bool_feature (*script_features);
}

static hb_ot_shape_normalization_mode_t
normalization_preference_default (const hb_segment_properties_t *props)
{
  switch ((hb_tag_t) props->script)
  {
    
    case HB_SCRIPT_HANGUL:
      return HB_OT_SHAPE_NORMALIZATION_MODE_COMPOSED_FULL;
  }
  return HB_OT_SHAPE_NORMALIZATION_MODE_COMPOSED_DIACRITICS;
}

static bool
compose_default (const hb_ot_shape_normalize_context_t *c,
		 hb_codepoint_t  a,
		 hb_codepoint_t  b,
		 hb_codepoint_t *ab)
{
  




  static const hb_codepoint_t sDageshForms[0x05EA - 0x05D0 + 1] = {
    0xFB30, 
    0xFB31, 
    0xFB32, 
    0xFB33, 
    0xFB34, 
    0xFB35, 
    0xFB36, 
    0x0000, 
    0xFB38, 
    0xFB39, 
    0xFB3A, 
    0xFB3B, 
    0xFB3C, 
    0x0000, 
    0xFB3E, 
    0x0000, 
    0xFB40, 
    0xFB41, 
    0x0000, 
    0xFB43, 
    0xFB44, 
    0x0000, 
    0xFB46, 
    0xFB47, 
    0xFB48, 
    0xFB49, 
    0xFB4A 
  };

  bool found = c->unicode->compose (a, b, ab);

  if (!found && (b & ~0x7F) == 0x0580) {
      

      switch (b) {
      case 0x05B4: 
	  if (a == 0x05D9) { 
	      *ab = 0xFB1D;
	      found = true;
	  }
	  break;
      case 0x05B7: 
	  if (a == 0x05F2) { 
	      *ab = 0xFB1F;
	      found = true;
	  } else if (a == 0x05D0) { 
	      *ab = 0xFB2E;
	      found = true;
	  }
	  break;
      case 0x05B8: 
	  if (a == 0x05D0) { 
	      *ab = 0xFB2F;
	      found = true;
	  }
	  break;
      case 0x05B9: 
	  if (a == 0x05D5) { 
	      *ab = 0xFB4B;
	      found = true;
	  }
	  break;
      case 0x05BC: 
	  if (a >= 0x05D0 && a <= 0x05EA) {
	      *ab = sDageshForms[a - 0x05D0];
	      found = (*ab != 0);
	  } else if (a == 0xFB2A) { 
	      *ab = 0xFB2C;
	      found = true;
	  } else if (a == 0xFB2B) { 
	      *ab = 0xFB2D;
	      found = true;
	  }
	  break;
      case 0x05BF: 
	  switch (a) {
	  case 0x05D1: 
	      *ab = 0xFB4C;
	      found = true;
	      break;
	  case 0x05DB: 
	      *ab = 0xFB4D;
	      found = true;
	      break;
	  case 0x05E4: 
	      *ab = 0xFB4E;
	      found = true;
	      break;
	  }
	  break;
      case 0x05C1: 
	  if (a == 0x05E9) { 
	      *ab = 0xFB2A;
	      found = true;
	  } else if (a == 0xFB49) { 
	      *ab = 0xFB2C;
	      found = true;
	  }
	  break;
      case 0x05C2: 
	  if (a == 0x05E9) { 
	      *ab = 0xFB2B;
	      found = true;
	  } else if (a == 0xFB49) { 
	      *ab = 0xFB2D;
	      found = true;
	  }
	  break;
      }
  }

  return found;
}

const hb_ot_complex_shaper_t _hb_ot_complex_shaper_default =
{
  "default",
  collect_features_default,
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  normalization_preference_default,
  NULL, 
  compose_default,
  NULL, 
  HB_OT_SHAPE_ZERO_WIDTH_MARKS_BY_UNICODE,
  true, 
};
