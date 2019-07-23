






































 
#ifndef nsCSSProperty_h___
#define nsCSSProperty_h___








enum nsCSSProperty {
  eCSSProperty_UNKNOWN = -1,

  #define CSS_PROP(name_, id_, method_, flags_, datastruct_, member_, type_, kwtable_, stylestruct_) eCSSProperty_##id_,
  #include "nsCSSPropList.h"
  #undef CSS_PROP

  eCSSProperty_COUNT_no_shorthands,
  
  eCSSProperty_COUNT_DUMMY = eCSSProperty_COUNT_no_shorthands - 1,

  #define CSS_PROP_SHORTHAND(name_, id_, method_, flags_) eCSSProperty_##id_,
  #include "nsCSSPropList.h"
  #undef CSS_PROP_SHORTHAND

  eCSSProperty_COUNT,

  
  

  
  
  eCSSPropertyExtra_no_properties,
  eCSSPropertyExtra_all_properties,

  
  eCSSPropertyExtra_x_none_value
};



enum nsCSSType {
  eCSSType_Value,
  eCSSType_Rect,
  eCSSType_ValuePair,
  eCSSType_ValueList,
  eCSSType_ValuePairList
};





enum nsCSSFontDesc {
  eCSSFontDesc_UNKNOWN = -1,
  eCSSFontDesc_Family,
  eCSSFontDesc_Style,
  eCSSFontDesc_Weight,
  eCSSFontDesc_Stretch,
  eCSSFontDesc_Src,
  eCSSFontDesc_UnicodeRange,
  eCSSFontDesc_COUNT
};

#endif 
