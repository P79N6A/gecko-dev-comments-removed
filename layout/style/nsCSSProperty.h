






































 
#ifndef nsCSSProperty_h___
#define nsCSSProperty_h___








enum nsCSSProperty {
  eCSSProperty_UNKNOWN = -1,

  #define CSS_PROP(name_, id_, method_, flags_, parsevariant_, kwtable_,     \
                   stylestruct_, stylestructoffset_, animtype_)              \
    eCSSProperty_##id_,
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

  
  eCSSPropertyExtra_x_none_value,
  eCSSPropertyExtra_x_auto_value
};



enum nsCSSFontDesc {
  eCSSFontDesc_UNKNOWN = -1,
#define CSS_FONT_DESC(name_, method_) eCSSFontDesc_##method_,
#include "nsCSSFontDescList.h"
#undef CSS_FONT_DESC
  eCSSFontDesc_COUNT
};

#endif 
