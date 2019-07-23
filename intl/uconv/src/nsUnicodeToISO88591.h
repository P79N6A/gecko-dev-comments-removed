




































#ifndef nsUnicodeToISO88591_h___
#define nsUnicodeToISO88591_h___

#include "nsISupports.h"


#define NS_UNICODETOISO88591_CID \
  { 0x920307b0, 0xc6e8, 0x11d2, {0x8a, 0xa8, 0x0, 0x60, 0x8, 0x11, 0xa8, 0x36}}

#define NS_UNICODETOISO88591_CONTRACTID "@mozilla.org/intl/unicode/encoder;1?charset=ISO-8859-1"











NS_METHOD
nsUnicodeToISO88591Constructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult);

#endif 
