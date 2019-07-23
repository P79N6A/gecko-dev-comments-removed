




































#ifndef nsCP1252ToUnicode_h___
#define nsCP1252ToUnicode_h___

#include "nsISupports.h"



#define NS_CP1252TOUNICODE_CID \
  { 0x7c657d15, 0xec5e, 0x11d2, {0x8a, 0xac, 0x0, 0x60, 0x8, 0x11, 0xa8, 0x36}}

#define NS_CP1252TOUNICODE_CONTRACTID "@mozilla.org/intl/unicode/decoder;1?charset=windows-1252"







NS_METHOD
nsCP1252ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult);

#endif 
