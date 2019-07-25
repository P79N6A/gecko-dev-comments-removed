











































#ifndef nsIXBLService_h__
#define nsIXBLService_h__

#include "nsISupports.h"

class nsIContent;
class nsIDocument;
class nsIDOMEventTarget;
class nsIDOMNodeList;
class nsXBLBinding;
class nsXBLDocumentInfo;
class nsIURI;
class nsIAtom;
class nsIPrincipal;

#define NS_IXBLSERVICE_IID      \
{ 0x8a25483c, 0x1ac6, 0x4796, { 0xa6, 0x12, 0x5a, 0xe0, 0x5c, 0x83, 0x65, 0x0b } }

class nsIXBLService : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXBLSERVICE_IID)

  
  
  NS_IMETHOD LoadBindings(nsIContent* aContent, nsIURI* aURL,
                          nsIPrincipal* aOriginPrincipal, PRBool aAugmentFlag,
                          nsXBLBinding** aBinding, PRBool* aResolveStyle) = 0;

  
  NS_IMETHOD BindingReady(nsIContent* aBoundElement, nsIURI* aURI, PRBool* aIsReady) = 0;

  
  NS_IMETHOD ResolveTag(nsIContent* aContent, PRInt32* aNameSpaceID, nsIAtom** aResult) = 0;

  
  
  
  NS_IMETHOD LoadBindingDocumentInfo(nsIContent* aBoundElement,
                                     nsIDocument* aBoundDocument,
                                     nsIURI* aBindingURI,
                                     nsIPrincipal* aOriginPrincipal,
                                     PRBool aForceSyncLoad,
                                     nsXBLDocumentInfo** aResult) = 0;

  
  NS_IMETHOD AttachGlobalKeyHandler(nsIDOMEventTarget* aTarget) = 0;
  NS_IMETHOD DetachGlobalKeyHandler(nsIDOMEventTarget* aTarget) = 0;
  
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIXBLService, NS_IXBLSERVICE_IID)

#endif 
