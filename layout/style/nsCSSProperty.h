





































 
#ifndef nsCSSProperty_h___
#define nsCSSProperty_h___








enum nsCSSProperty {
  eCSSProperty_UNKNOWN = -1,

  #define CSS_PROP(name_, id_, method_, datastruct_, member_, type_, kwtable_) eCSSProperty_##id_,
  #include "nsCSSPropList.h"
  #undef CSS_PROP

  eCSSProperty_COUNT_no_shorthands,
  
  eCSSProperty_COUNT_DUMMY = eCSSProperty_COUNT_no_shorthands - 1,

  #define CSS_PROP_SHORTHAND(name_, id_, method_) eCSSProperty_##id_,
  #include "nsCSSPropList.h"
  #undef CSS_PROP_SHORTHAND

  eCSSProperty_COUNT
};



enum nsCSSType {
  eCSSType_Value,
  eCSSType_Rect,
  eCSSType_ValuePair,
  eCSSType_ValueList,
  eCSSType_CounterData,
  eCSSType_Quotes
};

#endif 
