





























#ifndef HB_UNICODE_PRIVATE_HH
#define HB_UNICODE_PRIVATE_HH

#include "hb-private.hh"
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
    if (unlikely (!a || !b)) return false;
    return func.compose (this, a, b, ab, user_data.compose);
  }

  inline hb_bool_t decompose (hb_codepoint_t ab,
			      hb_codepoint_t *a, hb_codepoint_t *b)
  {
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


  inline unsigned int
  modified_combining_class (hb_codepoint_t unicode)
  {
    
    if (unlikely (unicode == 0x1037u)) unicode = 0x103Au;

    

    if (unlikely (unicode == 0x1A60u)) return 254;

    

    if (unlikely (unicode == 0x0FC6u)) return 254;

    return _hb_modified_combining_class[combining_class (unicode)];
  }

  static inline hb_bool_t
  is_variation_selector (hb_codepoint_t unicode)
  {
    

    return unlikely (hb_in_ranges (unicode,
				   0xFE00u, 0xFE0Fu, 
				   0xE0100u, 0xE01EFu));  
  }

  



































  static inline hb_bool_t
  is_default_ignorable (hb_codepoint_t ch)
  {
    hb_codepoint_t plane = ch >> 16;
    if (likely (plane == 0))
    {
      
      hb_codepoint_t page = ch >> 8;
      switch (page) {
	case 0x00: return unlikely (ch == 0x00ADu);
	case 0x03: return unlikely (ch == 0x034Fu);
	case 0x06: return unlikely (ch == 0x061Cu);
	case 0x17: return hb_in_range (ch, 0x17B4u, 0x17B5u);
	case 0x18: return hb_in_range (ch, 0x180Bu, 0x180Eu);
	case 0x20: return hb_in_ranges (ch, 0x200Bu, 0x200Fu,
							    0x202Au, 0x202Eu,
							    0x2060u, 0x206Fu);
	case 0xFE: return hb_in_range (ch, 0xFE00u, 0xFE0Fu) || ch == 0xFEFFu;
	case 0xFF: return hb_in_range (ch, 0xFFF0u, 0xFFF8u);
	default: return false;
      }
    }
    else
    {
      
      switch (plane) {
	case 0x01: return hb_in_ranges (ch, 0x1BCA0u, 0x1BCA3u,
					    0x1D173u, 0x1D17Au);
	case 0x0E: return hb_in_range (ch, 0xE0000u, 0xE0FFFu);
	default: return false;
      }
    }
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


extern HB_INTERNAL const hb_unicode_funcs_t _hb_unicode_funcs_nil;

















#define HB_MODIFIED_COMBINING_CLASS_CCC10 22 /* sheva */
#define HB_MODIFIED_COMBINING_CLASS_CCC11 15 /* hataf segol */
#define HB_MODIFIED_COMBINING_CLASS_CCC12 16 /* hataf patah */
#define HB_MODIFIED_COMBINING_CLASS_CCC13 17 /* hataf qamats */
#define HB_MODIFIED_COMBINING_CLASS_CCC14 23 /* hiriq */
#define HB_MODIFIED_COMBINING_CLASS_CCC15 18 /* tsere */
#define HB_MODIFIED_COMBINING_CLASS_CCC16 19 /* segol */
#define HB_MODIFIED_COMBINING_CLASS_CCC17 20 /* patah */
#define HB_MODIFIED_COMBINING_CLASS_CCC18 21 /* qamats */
#define HB_MODIFIED_COMBINING_CLASS_CCC19 14 /* holam */
#define HB_MODIFIED_COMBINING_CLASS_CCC20 24 /* qubuts */
#define HB_MODIFIED_COMBINING_CLASS_CCC21 12 /* dagesh */
#define HB_MODIFIED_COMBINING_CLASS_CCC22 25 /* meteg */
#define HB_MODIFIED_COMBINING_CLASS_CCC23 13 /* rafe */
#define HB_MODIFIED_COMBINING_CLASS_CCC24 10 /* shin dot */
#define HB_MODIFIED_COMBINING_CLASS_CCC25 11 /* sin dot */
#define HB_MODIFIED_COMBINING_CLASS_CCC26 26 /* point varika */








#define HB_MODIFIED_COMBINING_CLASS_CCC27 28 /* fathatan */
#define HB_MODIFIED_COMBINING_CLASS_CCC28 29 /* dammatan */
#define HB_MODIFIED_COMBINING_CLASS_CCC29 30 /* kasratan */
#define HB_MODIFIED_COMBINING_CLASS_CCC30 31 /* fatha */
#define HB_MODIFIED_COMBINING_CLASS_CCC31 32 /* damma */
#define HB_MODIFIED_COMBINING_CLASS_CCC32 33 /* kasra */
#define HB_MODIFIED_COMBINING_CLASS_CCC33 27 /* shadda */
#define HB_MODIFIED_COMBINING_CLASS_CCC34 34 /* sukun */
#define HB_MODIFIED_COMBINING_CLASS_CCC35 35 /* superscript alef */


#define HB_MODIFIED_COMBINING_CLASS_CCC36 36 /* superscript alaph */








#define HB_MODIFIED_COMBINING_CLASS_CCC84 0 /* length mark */
#define HB_MODIFIED_COMBINING_CLASS_CCC91 0 /* ai length mark */







#define HB_MODIFIED_COMBINING_CLASS_CCC103 3 /* sara u / sara uu */
#define HB_MODIFIED_COMBINING_CLASS_CCC107 107 /* mai * */


#define HB_MODIFIED_COMBINING_CLASS_CCC118 118 /* sign u / sign uu */
#define HB_MODIFIED_COMBINING_CLASS_CCC122 122 /* mai * */


#define HB_MODIFIED_COMBINING_CLASS_CCC129 129 /* sign aa */
#define HB_MODIFIED_COMBINING_CLASS_CCC130 130 /* sign i */
#define HB_MODIFIED_COMBINING_CLASS_CCC132 132 /* sign u */




#define HB_UNICODE_GENERAL_CATEGORY_IS_MARK(gen_cat) \
	(FLAG (gen_cat) & \
	 (FLAG (HB_UNICODE_GENERAL_CATEGORY_SPACING_MARK) | \
	  FLAG (HB_UNICODE_GENERAL_CATEGORY_ENCLOSING_MARK) | \
	  FLAG (HB_UNICODE_GENERAL_CATEGORY_NON_SPACING_MARK)))


#endif 
