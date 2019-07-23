




































#ifndef nsMacRomanToUnicode_h___
#define nsMacRomanToUnicode_h___

#include "nsISupports.h"



#define NS_MACROMANTOUNICODE_CID \
  { 0x7b8556a1, 0xec79, 0x11d2, {0x8a, 0xac, 0x0, 0x60, 0x8, 0x11, 0xa8, 0x36}}

#define NS_MACROMANTOUNICODE_CONTRACTID "@mozilla.org/intl/unicode/decoder;1?charset=x-mac-roman"










NS_METHOD
nsMacRomanToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult);

#endif 
