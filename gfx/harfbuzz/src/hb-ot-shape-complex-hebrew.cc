

























#include "hb-ot-shape-complex-private.hh"


static bool
compose_hebrew (const hb_ot_shape_normalize_context_t *c,
		hb_codepoint_t  a,
		hb_codepoint_t  b,
		hb_codepoint_t *ab)
{
  




  static const hb_codepoint_t sDageshForms[0x05EAu - 0x05D0u + 1] = {
    0xFB30u, 
    0xFB31u, 
    0xFB32u, 
    0xFB33u, 
    0xFB34u, 
    0xFB35u, 
    0xFB36u, 
    0x0000u, 
    0xFB38u, 
    0xFB39u, 
    0xFB3Au, 
    0xFB3Bu, 
    0xFB3Cu, 
    0x0000u, 
    0xFB3Eu, 
    0x0000u, 
    0xFB40u, 
    0xFB41u, 
    0x0000u, 
    0xFB43u, 
    0xFB44u, 
    0x0000u, 
    0xFB46u, 
    0xFB47u, 
    0xFB48u, 
    0xFB49u, 
    0xFB4Au 
  };

  bool found = c->unicode->compose (a, b, ab);

  if (!found && !c->plan->has_mark)
  {
      

      switch (b) {
      case 0x05B4u: 
	  if (a == 0x05D9u) { 
	      *ab = 0xFB1Du;
	      found = true;
	  }
	  break;
      case 0x05B7u: 
	  if (a == 0x05F2u) { 
	      *ab = 0xFB1Fu;
	      found = true;
	  } else if (a == 0x05D0u) { 
	      *ab = 0xFB2Eu;
	      found = true;
	  }
	  break;
      case 0x05B8u: 
	  if (a == 0x05D0u) { 
	      *ab = 0xFB2Fu;
	      found = true;
	  }
	  break;
      case 0x05B9u: 
	  if (a == 0x05D5u) { 
	      *ab = 0xFB4Bu;
	      found = true;
	  }
	  break;
      case 0x05BCu: 
	  if (a >= 0x05D0u && a <= 0x05EAu) {
	      *ab = sDageshForms[a - 0x05D0u];
	      found = (*ab != 0);
	  } else if (a == 0xFB2Au) { 
	      *ab = 0xFB2Cu;
	      found = true;
	  } else if (a == 0xFB2Bu) { 
	      *ab = 0xFB2Du;
	      found = true;
	  }
	  break;
      case 0x05BFu: 
	  switch (a) {
	  case 0x05D1u: 
	      *ab = 0xFB4Cu;
	      found = true;
	      break;
	  case 0x05DBu: 
	      *ab = 0xFB4Du;
	      found = true;
	      break;
	  case 0x05E4u: 
	      *ab = 0xFB4Eu;
	      found = true;
	      break;
	  }
	  break;
      case 0x05C1u: 
	  if (a == 0x05E9u) { 
	      *ab = 0xFB2Au;
	      found = true;
	  } else if (a == 0xFB49u) { 
	      *ab = 0xFB2Cu;
	      found = true;
	  }
	  break;
      case 0x05C2u: 
	  if (a == 0x05E9u) { 
	      *ab = 0xFB2Bu;
	      found = true;
	  } else if (a == 0xFB49u) { 
	      *ab = 0xFB2Du;
	      found = true;
	  }
	  break;
      }
  }

  return found;
}


const hb_ot_complex_shaper_t _hb_ot_complex_shaper_hebrew =
{
  "hebrew",
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  HB_OT_SHAPE_NORMALIZATION_MODE_DEFAULT,
  NULL, 
  compose_hebrew,
  NULL, 
  HB_OT_SHAPE_ZERO_WIDTH_MARKS_BY_GDEF_LATE,
  true, 
};
