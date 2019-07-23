



































#ifndef nsITextTransform_h__
#define nsITextTransform_h__


#include "nsISupports.h"
#include "nsString.h"
#include "nscore.h"


#define NS_ITEXTTRANSFORM_IID \
{ 0xccd4d371, 0xccdc, 0x11d2, \
    { 0xb3, 0xb1, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }

#define NS_TEXTTRANSFORM_CONTRACTID_BASE "@mozilla.org/intl/texttransform;1?type="

class nsITextTransform : public nsISupports {

public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ITEXTTRANSFORM_IID)

  NS_IMETHOD Change( const PRUnichar* aText, PRInt32 aTextLength, nsString& aResult) = 0;
  NS_IMETHOD Change( nsString& aText, nsString& aResult) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsITextTransform, NS_ITEXTTRANSFORM_IID)

#endif  
