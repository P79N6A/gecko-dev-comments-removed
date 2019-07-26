

























#ifndef HB_OT_SHAPE_COMPLEX_PRIVATE_HH
#define HB_OT_SHAPE_COMPLEX_PRIVATE_HH

#include "hb-private.hh"

#include "hb-ot-shape-private.hh"
#include "hb-ot-shape-normalize-private.hh"




#define complex_var_u8_0()	var2.u8[2]
#define complex_var_u8_1()	var2.u8[3]


enum hb_ot_shape_zero_width_marks_type_t {
  HB_OT_SHAPE_ZERO_WIDTH_MARKS_NONE,
  HB_OT_SHAPE_ZERO_WIDTH_MARKS_BY_UNICODE,
  HB_OT_SHAPE_ZERO_WIDTH_MARKS_BY_GDEF
};



#define HB_COMPLEX_SHAPERS_IMPLEMENT_SHAPERS \
  HB_COMPLEX_SHAPER_IMPLEMENT (default) /* should be first */ \
  HB_COMPLEX_SHAPER_IMPLEMENT (arabic) \
  HB_COMPLEX_SHAPER_IMPLEMENT (indic) \
  HB_COMPLEX_SHAPER_IMPLEMENT (myanmar) \
  HB_COMPLEX_SHAPER_IMPLEMENT (sea) \
  HB_COMPLEX_SHAPER_IMPLEMENT (thai) \
  /* ^--- Add new shapers here */


struct hb_ot_complex_shaper_t
{
  char name[8];

  




  void (*collect_features) (hb_ot_shape_planner_t *plan);

  





  void (*override_features) (hb_ot_shape_planner_t *plan);


  




  void *(*data_create) (const hb_ot_shape_plan_t *plan);

  





  void (*data_destroy) (void *data);


  




  void (*preprocess_text) (const hb_ot_shape_plan_t *plan,
			   hb_buffer_t              *buffer,
			   hb_font_t                *font);


  



  hb_ot_shape_normalization_mode_t
  (*normalization_preference) (const hb_segment_properties_t *props);

  



  bool (*decompose) (const hb_ot_shape_normalize_context_t *c,
		     hb_codepoint_t  ab,
		     hb_codepoint_t *a,
		     hb_codepoint_t *b);

  



  bool (*compose) (const hb_ot_shape_normalize_context_t *c,
		   hb_codepoint_t  a,
		   hb_codepoint_t  b,
		   hb_codepoint_t *ab);

  





  void (*setup_masks) (const hb_ot_shape_plan_t *plan,
		       hb_buffer_t              *buffer,
		       hb_font_t                *font);

  hb_ot_shape_zero_width_marks_type_t zero_width_marks;

  bool fallback_position;
};

#define HB_COMPLEX_SHAPER_IMPLEMENT(name) extern HB_INTERNAL const hb_ot_complex_shaper_t _hb_ot_complex_shaper_##name;
HB_COMPLEX_SHAPERS_IMPLEMENT_SHAPERS
#undef HB_COMPLEX_SHAPER_IMPLEMENT


static inline const hb_ot_complex_shaper_t *
hb_ot_shape_complex_categorize (const hb_ot_shape_planner_t *planner)
{
  switch ((hb_tag_t) planner->props.script)
  {
    default:
      return &_hb_ot_complex_shaper_default;


    
    case HB_SCRIPT_ARABIC:
    case HB_SCRIPT_MONGOLIAN:
    case HB_SCRIPT_SYRIAC:

    
    case HB_SCRIPT_NKO:
    case HB_SCRIPT_PHAGS_PA:

    
    case HB_SCRIPT_MANDAIC:

      

      if (planner->map.chosen_script[0] != HB_OT_TAG_DEFAULT_SCRIPT ||
	  planner->props.script == HB_SCRIPT_ARABIC)
	return &_hb_ot_complex_shaper_arabic;
      else
	return &_hb_ot_complex_shaper_default;


    
    case HB_SCRIPT_THAI:
    case HB_SCRIPT_LAO:

      return &_hb_ot_complex_shaper_thai;



    


#if 0
    











    

    
    case HB_SCRIPT_BUHID:
    case HB_SCRIPT_HANUNOO:

    
    case HB_SCRIPT_SAURASHTRA:

    
    case HB_SCRIPT_BATAK:
    case HB_SCRIPT_BRAHMI:


    

    
    
    case HB_SCRIPT_LAO:
    case HB_SCRIPT_THAI:

    
    case HB_SCRIPT_TIBETAN:

    
    case HB_SCRIPT_TAGALOG:
    case HB_SCRIPT_TAGBANWA:

    
    case HB_SCRIPT_LIMBU:
    case HB_SCRIPT_TAI_LE:

    
    case HB_SCRIPT_KHAROSHTHI:
    case HB_SCRIPT_SYLOTI_NAGRI:

    
    case HB_SCRIPT_KAYAH_LI:

    
    case HB_SCRIPT_TAI_VIET:


#endif

    
    case HB_SCRIPT_BENGALI:
    case HB_SCRIPT_DEVANAGARI:
    case HB_SCRIPT_GUJARATI:
    case HB_SCRIPT_GURMUKHI:
    case HB_SCRIPT_KANNADA:
    case HB_SCRIPT_MALAYALAM:
    case HB_SCRIPT_ORIYA:
    case HB_SCRIPT_TAMIL:
    case HB_SCRIPT_TELUGU:

    
    case HB_SCRIPT_SINHALA:

    
    case HB_SCRIPT_BUGINESE:

    
    case HB_SCRIPT_BALINESE:

    
    case HB_SCRIPT_LEPCHA:
    case HB_SCRIPT_REJANG:
    case HB_SCRIPT_SUNDANESE:

    
    case HB_SCRIPT_JAVANESE:
    case HB_SCRIPT_KAITHI:
    case HB_SCRIPT_MEETEI_MAYEK:

    

    
    case HB_SCRIPT_CHAKMA:
    case HB_SCRIPT_SHARADA:
    case HB_SCRIPT_TAKRI:

      



      if (planner->map.chosen_script[0] == HB_TAG ('D','F','L','T'))
	return &_hb_ot_complex_shaper_default;
      else
	return &_hb_ot_complex_shaper_indic;

    case HB_SCRIPT_KHMER:
      




      if (planner->map.found_script[0] &&
	  hb_ot_layout_language_find_feature (planner->face, HB_OT_TAG_GSUB,
					      planner->map.script_index[0],
					      planner->map.language_index[0],
					      HB_TAG ('p','r','e','f'),
					      NULL))
	return &_hb_ot_complex_shaper_indic;
      else
	return &_hb_ot_complex_shaper_default;

    case HB_SCRIPT_MYANMAR:
      

      if (planner->map.chosen_script[0] == HB_TAG ('m','y','m','2'))
	return &_hb_ot_complex_shaper_myanmar;
      else
	return &_hb_ot_complex_shaper_default;

    
    case HB_SCRIPT_NEW_TAI_LUE:

    
    case HB_SCRIPT_CHAM:

    
    case HB_SCRIPT_TAI_THAM:

      



      if (planner->map.chosen_script[0] == HB_TAG ('D','F','L','T'))
	return &_hb_ot_complex_shaper_default;
      else
	return &_hb_ot_complex_shaper_sea;
  }
}


#endif 
