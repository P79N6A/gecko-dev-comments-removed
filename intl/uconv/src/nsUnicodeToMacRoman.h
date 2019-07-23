




































#ifndef nsUnicodeToMacRoman_h___
#define nsUnicodeToMacRoman_h___

#include "nsISupports.h"



#define NS_UNICODETOMACROMAN_CID \
  { 0x7b8556af, 0xec79, 0x11d2, {0x8a, 0xac, 0x0, 0x60, 0x8, 0x11, 0xa8, 0x36}}

#define NS_UNICODETOMACROMAN_CONTRACTID "@mozilla.org/intl/unicode/encoder;1?charset=x-mac-roman"







NS_METHOD
nsUnicodeToMacRomanConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult);

#endif 
