





























#ifndef HB_UNICODE_PRIVATE_HH
#define HB_UNICODE_PRIVATE_HH

#include "hb-private.hh"

#include "hb-unicode.h"
#include "hb-object-private.hh"


extern HB_INTERNAL const uint8_t _hb_modified_combining_class[256];





#define HB_UNICODE_FUNCS_IMPLEMENT_CALLBACKS \
  HB_UNICODE_FUNC_IMPLEMENT (combining_class) \
  HB_UNICODE_FUNC_IMPLEMENT (eastasian_width) \
  HB_UNICODE_FUNC_IMPLEMENT (general_category) \
  HB_UNICODE_FUNC_IMPLEMENT (mirroring) \
  HB_UNICODE_FUNC_IMPLEMENT (script) \
  HB_UNICODE_FUNC_IMPLEMENT (compose) \
  HB_UNICODE_FUNC_IMPLEMENT (decompose) \
  HB_UNICODE_FUNC_IMPLEMENT (decompose_compatibility) \
  /* ^--- Add new callbacks here */


#define HB_UNICODE_FUNCS_IMPLEMENT_CALLBACKS_SIMPLE \
  HB_UNICODE_FUNC_IMPLEMENT (hb_unicode_combining_class_t, combining_class) \
  HB_UNICODE_FUNC_IMPLEMENT (unsigned int, eastasian_width) \
  HB_UNICODE_FUNC_IMPLEMENT (hb_unicode_general_category_t, general_category) \
  HB_UNICODE_FUNC_IMPLEMENT (hb_codepoint_t, mirroring) \
  HB_UNICODE_FUNC_IMPLEMENT (hb_script_t, script) \
  /* ^--- Add new simple callbacks here */

struct hb_unicode_funcs_t {
  hb_object_header_t header;
  ASSERT_POD ();

  hb_unicode_funcs_t *parent;

  bool immutable;

#define HB_UNICODE_FUNC_IMPLEMENT(return_type, name) \
  inline return_type name (hb_codepoint_t unicode) { return func.name (this, unicode, user_data.name); }
HB_UNICODE_FUNCS_IMPLEMENT_CALLBACKS_SIMPLE
#undef HB_UNICODE_FUNC_IMPLEMENT

  inline hb_bool_t compose (hb_codepoint_t a, hb_codepoint_t b,
			    hb_codepoint_t *ab)
  {
    *ab = 0;
    
    if ((FLAG (general_category (a)) &
	 (FLAG (HB_UNICODE_GENERAL_CATEGORY_SPACING_MARK) |
	  FLAG (HB_UNICODE_GENERAL_CATEGORY_ENCLOSING_MARK) |
	  FLAG (HB_UNICODE_GENERAL_CATEGORY_NON_SPACING_MARK))))
      return false;
    
    if (a == 0x09AF && b == 0x09BC) { *ab = 0x09DF; return true; }
    return func.compose (this, a, b, ab, user_data.compose);
  }

  inline hb_bool_t decompose (hb_codepoint_t ab,
			      hb_codepoint_t *a, hb_codepoint_t *b)
  {
    
    switch (ab) {
      case 0x0AC9  : return false;

      case 0x0931  : return false;
      case 0x0B94  : return false;

      

      case 0x0DDA  : *a = 0x0DD9; *b= 0x0DDA; return true;
      case 0x0DDC  : *a = 0x0DD9; *b= 0x0DDC; return true;
      case 0x0DDD  : *a = 0x0DD9; *b= 0x0DDD; return true;
      case 0x0DDE  : *a = 0x0DD9; *b= 0x0DDE; return true;

      case 0x0F77  : *a = 0x0FB2; *b= 0x0F81; return true;
      case 0x0F79  : *a = 0x0FB3; *b= 0x0F81; return true;
      case 0x17BE  : *a = 0x17C1; *b= 0x17BE; return true;
      case 0x17BF  : *a = 0x17C1; *b= 0x17BF; return true;
      case 0x17C0  : *a = 0x17C1; *b= 0x17C0; return true;
      case 0x17C4  : *a = 0x17C1; *b= 0x17C4; return true;
      case 0x17C5  : *a = 0x17C1; *b= 0x17C5; return true;
      case 0x1925  : *a = 0x1920; *b= 0x1923; return true;
      case 0x1926  : *a = 0x1920; *b= 0x1924; return true;
      case 0x1B3C  : *a = 0x1B42; *b= 0x1B3C; return true;
      case 0x1112E  : *a = 0x11127; *b= 0x11131; return true;
      case 0x1112F  : *a = 0x11127; *b= 0x11132; return true;
#if 0
      case 0x0B57  : *a = 0xno decomp, -> RIGHT; return true;
      case 0x1C29  : *a = 0xno decomp, -> LEFT; return true;
      case 0xA9C0  : *a = 0xno decomp, -> RIGHT; return true;
      case 0x111BF  : *a = 0xno decomp, -> ABOVE; return true;
#endif
    }
    *a = ab; *b = 0;
    return func.decompose (this, ab, a, b, user_data.decompose);
  }

  inline unsigned int decompose_compatibility (hb_codepoint_t  u,
					       hb_codepoint_t *decomposed)
  {
    unsigned int ret = func.decompose_compatibility (this, u, decomposed, user_data.decompose_compatibility);
    if (ret == 1 && u == decomposed[0]) {
      decomposed[0] = 0;
      return 0;
    }
    decomposed[ret] = 0;
    return ret;
  }


  unsigned int
  modified_combining_class (hb_codepoint_t unicode)
  {
    return _hb_modified_combining_class[combining_class (unicode)];
  }

  inline hb_bool_t
  is_variation_selector (hb_codepoint_t unicode)
  {
    return unlikely (hb_in_ranges<hb_codepoint_t> (unicode,
						   0x180B, 0x180D, 
						   0xFE00, 0xFE0F, 
						   0xE0100, 0xE01EF));  
  }

  



























  inline hb_bool_t
  is_zero_width (hb_codepoint_t ch)
  {
    return ((ch & ~0x007F) == 0x2000 && (hb_in_ranges<hb_codepoint_t> (ch,
								       0x200B, 0x200F,
								       0x202A, 0x202E,
								       0x2060, 0x2064) ||
					 (ch == 0x2028))) ||
	    unlikely (ch == 0x0009 ||
		      ch == 0x00AD ||
		      ch == 0x034F ||
		      ch == 0x180E ||
		      ch == 0xFEFF);
  }


  struct {
#define HB_UNICODE_FUNC_IMPLEMENT(name) hb_unicode_##name##_func_t name;
    HB_UNICODE_FUNCS_IMPLEMENT_CALLBACKS
#undef HB_UNICODE_FUNC_IMPLEMENT
  } func;

  struct {
#define HB_UNICODE_FUNC_IMPLEMENT(name) void *name;
    HB_UNICODE_FUNCS_IMPLEMENT_CALLBACKS
#undef HB_UNICODE_FUNC_IMPLEMENT
  } user_data;

  struct {
#define HB_UNICODE_FUNC_IMPLEMENT(name) hb_destroy_func_t name;
    HB_UNICODE_FUNCS_IMPLEMENT_CALLBACKS
#undef HB_UNICODE_FUNC_IMPLEMENT
  } destroy;
};


#ifdef HAVE_GLIB
extern HB_INTERNAL const hb_unicode_funcs_t _hb_glib_unicode_funcs;
#define _hb_unicode_funcs_default _hb_glib_unicode_funcs
#elif defined(HAVE_ICU)
extern HB_INTERNAL const hb_unicode_funcs_t _hb_icu_unicode_funcs;
#define _hb_unicode_funcs_default _hb_icu_unicode_funcs
#else
#define HB_UNICODE_FUNCS_NIL 1
extern HB_INTERNAL const hb_unicode_funcs_t _hb_unicode_funcs_nil;
#define _hb_unicode_funcs_default _hb_unicode_funcs_nil
#endif


#endif 
