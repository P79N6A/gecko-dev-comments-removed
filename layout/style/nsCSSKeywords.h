






































#ifndef nsCSSKeywords_h___
#define nsCSSKeywords_h___

#include "nsStringFwd.h"








#define CSS_KEY(_name,_id) eCSSKeyword_##_id,
enum nsCSSKeyword {
  eCSSKeyword_UNKNOWN = -1,
#include "nsCSSKeywordList.h"
  eCSSKeyword_COUNT
};
#undef CSS_KEY


class nsCSSKeywords {
public:
  static void AddRefTable(void);
  static void ReleaseTable(void);

  
  static nsCSSKeyword LookupKeyword(const nsACString& aKeyword);
  static nsCSSKeyword LookupKeyword(const nsAString& aKeyword);

  
  static const nsAFlatCString& GetStringValue(nsCSSKeyword aKeyword);
};

#endif 
