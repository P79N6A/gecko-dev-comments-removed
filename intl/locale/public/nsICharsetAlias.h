




































#ifndef nsICharsetAlias_h___
#define nsICharsetAlias_h___

#include "nscore.h"
#include "nsStringGlue.h"
#include "nsISupports.h"


#define NS_ICHARSETALIAS_IID \
{   0x0b4028d6, \
    0x7473, \
    0x4958, \
    {0x9b, 0x3c, 0x4d, 0xee, 0x46, 0xbf, 0x68, 0xcb} }


#define NS_CHARSETALIAS_CID \
{ 0x98d41c21, 0xccf3, 0x11d2, { 0xb3, 0xb1, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 }}

#define NS_CHARSETALIAS_CONTRACTID "@mozilla.org/intl/charsetalias;1"

class nsICharsetAlias : public nsISupports
{
public:
   NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICHARSETALIAS_IID)

   NS_IMETHOD GetPreferred(const nsACString& aAlias, nsACString& aResult) = 0;
   NS_IMETHOD Equals(const nsACString& aCharset1, const nsACString& aCharset2, PRBool* aResult) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICharsetAlias, NS_ICHARSETALIAS_IID)

#endif 
