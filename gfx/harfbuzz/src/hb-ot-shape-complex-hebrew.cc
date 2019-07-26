

























#include "hb-ot-shape-complex-private.hh"


static bool
compose_hebrew (const hb_ot_shape_normalize_context_t *c,
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

  if (!found)
  {
      

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
  HB_OT_SHAPE_ZERO_WIDTH_MARKS_DEFAULT,
  true, 
};
