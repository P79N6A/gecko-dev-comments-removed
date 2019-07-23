











































#ifndef nsIXBLService_h__
#define nsIXBLService_h__

#include "nsISupports.h"

class nsIContent;
class nsIDocument;
class nsPIDOMEventTarget;
class nsIDOMNodeList;
class nsXBLBinding;
class nsIXBLDocumentInfo;
class nsIURI;
class nsIAtom;

#define NS_IXBLSERVICE_IID      \
  { 0xefda61b3, 0x5d04, 0x43b0, \
    { 0x98, 0x0c, 0x32, 0x62, 0x72, 0xc8, 0x5c, 0x68 } }

class nsIXBLService : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXBLSERVICE_IID)

  
  
  NS_IMETHOD LoadBindings(nsIContent* aContent, nsIURI* aURL, PRBool aAugmentFlag,
                          nsXBLBinding** aBinding, PRBool* aResolveStyle) = 0;

  
  NS_IMETHOD BindingReady(nsIContent* aBoundElement, nsIURI* aURI, PRBool* aIsReady) = 0;

  
  NS_IMETHOD ResolveTag(nsIContent* aContent, PRInt32* aNameSpaceID, nsIAtom** aResult) = 0;

  
  NS_IMETHOD LoadBindingDocumentInfo(nsIContent* aBoundElement, nsIDocument* aBoundDocument,
                                     nsIURI* aBindingURI,
                                     PRBool aForceSyncLoad, nsIXBLDocumentInfo** aResult) = 0;

  
  NS_IMETHOD AttachGlobalKeyHandler(nsPIDOMEventTarget* aTarget)=0;
  
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIXBLService, NS_IXBLSERVICE_IID)

#endif 
