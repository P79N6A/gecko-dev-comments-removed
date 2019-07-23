




































#ifndef nsMacUnicodeFontInfo_h__
#define nsMacUnicodeFontInfo_h__

#include "nscore.h"
class nsString;
class nsIUnicodeEncoder;

class nsMacUnicodeFontInfo 
{
public:
  PRBool HasGlyphFor(PRUnichar aChar);
  static void FreeGlobals();
  static nsresult GetConverterAndCCMap(const nsString& aFontName, nsIUnicodeEncoder** aConverter, PRUint16** aCCMap);
};

#endif 