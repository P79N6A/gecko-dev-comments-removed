











#ifndef nsIFormProcessor_h__
#define nsIFormProcessor_h__

#include "nsISupports.h"
#include "nsTArrayForwardDeclare.h"

class nsString;
class nsIDOMHTMLElement;


#define NS_FORMPROCESSOR_CID \
{ 0x0ae53c0f, 0x8ea2, 0x4916, { 0xbe, 0xdc, 0x71, 0x74, 0x43, 0xc3, 0xe1, 0x85 } }

#define NS_FORMPROCESSOR_CONTRACTID "@mozilla.org/layout/form-processor;1"


#define NS_IFORMPROCESSOR_IID      \
{ 0xbf8b1986, 0x8800, 0x424b, \
  { 0xb1, 0xe5, 0x7a, 0x2c, 0xa8, 0xb9, 0xe7, 0x6c } }







class nsIFormProcessor : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFORMPROCESSOR_IID)

  











  virtual nsresult ProcessValue(nsIDOMHTMLElement* aElement,
                                const nsAString& aName,
                                nsAString& aValue) = 0;

  



  virtual nsresult ProcessValueIPC(const nsAString& aOldValue,
                                   const nsAString& aKeyType,
                                   const nsAString& aChallenge,
                                   const nsAString& aKeyParams,
                                   nsAString& newValue) = 0;

  








  virtual nsresult ProvideContent(const nsAString& aFormType,
                                  nsTArray<nsString>& aContent,
                                  nsAString& aAttribute) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFormProcessor, NS_IFORMPROCESSOR_IID)

#endif 

