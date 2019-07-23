



































#ifndef nsIOrderIdFormater_h__
#define nsIOrderIdFormater_h__


#include "nsISupports.h"
#include "nsString.h"
#include "nscore.h"


#define NS_IORDERIDFORMATER_IID \
{ 0xccd4d372, 0xccdc, 0x11d2, \
    { 0xb3, 0xb1, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } };



class nsIOrderIdFormater : public nsISupports {

public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IORDERIDFORMATER_IID)

  
  NS_IMETHOD ToString( PRUint32 aOrder, nsString& aResult) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIOrderIdFormater, NS_IORDERIDFORMATER_IID)

#endif  
