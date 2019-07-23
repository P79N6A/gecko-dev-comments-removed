











































#ifndef nsIFormProcessor_h__
#define nsIFormProcessor_h__

#include "nsISupports.h"
#include "prtypes.h"

#include "nsIDOMHTMLInputElement.h"
#include "nsTArray.h"

class nsString;


#define NS_FORMPROCESSOR_CID \
{ 0x0ae53c0f, 0x8ea2, 0x4916, { 0xbe, 0xdc, 0x71, 0x74, 0x43, 0xc3, 0xe1, 0x85 } }

#define NS_FORMPROCESSOR_CONTRACTID "@mozilla.org/layout/form-processor;1"


#define NS_IFORMPROCESSOR_IID      \
{ 0x6d4ea1aa, 0xa6b2, 0x43bd, \
 { 0xa1, 0x9d, 0x3f, 0x0f, 0x26, 0x75, 0x0d, 0xf3 } }










class nsIFormProcessor : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFORMPROCESSOR_IID)

  











  NS_IMETHOD ProcessValue(nsIDOMHTMLElement *aElement, 
                          const nsAString& aName,
                          nsAString& aValue) = 0;

  








  NS_IMETHOD ProvideContent(const nsAString& aFormType, 
                            nsTArray<nsString>& aContent,
                            nsAString& aAttribute) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFormProcessor, NS_IFORMPROCESSOR_IID)

#endif 

