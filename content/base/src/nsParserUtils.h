








































#ifndef nsParserUtils_h__
#define nsParserUtils_h__

#include "nsString.h"
class nsIAtom;

class nsParserUtils {
public:
  













  static bool
  GetQuotedAttributeValue(const nsString& aSource, nsIAtom *aName,
                          nsAString& aValue);

  static bool
  IsJavaScriptLanguage(const nsString& aName, PRUint32 *aVerFlags);

  static void
  SplitMimeType(const nsAString& aValue, nsString& aType,
                nsString& aParams);
};

#endif 



