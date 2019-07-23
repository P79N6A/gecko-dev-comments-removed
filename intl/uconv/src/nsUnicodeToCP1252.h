




































#ifndef nsUnicodeToCP1252_h___
#define nsUnicodeToCP1252_h___

#include "nsISupports.h"


#define NS_UNICODETOCP1252_CID \
  { 0x7b8556ac, 0xec79, 0x11d2, {0x8a, 0xac, 0x0, 0x60, 0x8, 0x11, 0xa8, 0x36}}

#define NS_UNICODETOCP1252_CONTRACTID "@mozilla.org/intl/unicode/encoder;1?charset=windows-1252"










NS_METHOD
nsUnicodeToCP1252Constructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult);

#endif 
