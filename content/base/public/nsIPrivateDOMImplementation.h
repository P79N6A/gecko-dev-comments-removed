




































#ifndef nsIPrivateDOMImplementation_h__
#define nsIPrivateDOMImplementation_h__

#include "nsISupports.h"

class nsIURI;
class nsIPrincipal;




#define NS_IPRIVATEDOMIMPLEMENTATION_IID \
{ /* 87c20441-8b0d-4383-a189-52fef1dd5d8a */ \
0x87c20441, 0x8b0d, 0x4383, \
 { 0xa1, 0x89, 0x52, 0xfe, 0xf1, 0xdd, 0x5d, 0x8a } }

class nsIPrivateDOMImplementation : public nsISupports {

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPRIVATEDOMIMPLEMENTATION_IID)

  NS_IMETHOD Init(nsIURI* aDocumentURI, nsIURI* aBaseURI,
                  nsIPrincipal* aPrincipal) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIPrivateDOMImplementation,
                              NS_IPRIVATEDOMIMPLEMENTATION_IID)

nsresult
NS_NewDOMImplementation(nsIDOMDOMImplementation** aInstancePtrResult);

#endif 
